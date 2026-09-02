// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include "SonoUtility.h"
#include <utility>

SoundboardProcessor::SoundboardProcessor(SoundboardChannelProcessor* channelProcessor, File supportDir) : channelProcessor(
        channelProcessor)
{
    soundboardsFile = supportDir.getChildFile("soundboards.xml");

    loadFromDisk();
}

SoundboardProcessor::~SoundboardProcessor()
{
    saveToDisk();

    if (channelProcessor == nullptr)
        return;

    // Playback managers outlive this view/controller. Detach their listeners
    // and sample pointers before the heap-owned samples are destroyed.
    for (auto& soundboard : soundboards) {
        for (auto& sample : soundboard.getSamples()) {
            if (sample == nullptr)
                continue;

            if (auto manager = channelProcessor->findPlaybackManager(*sample))
                (*manager)->detach(this);

            channelProcessor->removeSample(*sample);
        }
    }
}

Soundboard& SoundboardProcessor::addSoundboard(const String& name, const bool select)
{
    auto newSoundboard = Soundboard(name);
    soundboards.push_back(std::move(newSoundboard));

    if (select) {
        selectedSoundboardIndex = static_cast<int>(getNumberOfSoundboards()) - 1;
    }

    reorderSoundboards();
    saveToDisk();

    return soundboards[selectedSoundboardIndex.value_or(0)];
}

void SoundboardProcessor::renameSoundboard(int index, String newName)
{
    if (index < 0 || index >= static_cast<int>(soundboards.size()))
        return;

    auto& toRename = soundboards[index];
    toRename.setName(std::move(newName));

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::deleteSoundboard(int index)
{
    if (index < 0 || index >= static_cast<int>(soundboards.size()))
        return;

    std::vector<juce::URL> urls;
    
    for (const auto& samplePtr : soundboards[index].getSamples()) {
        if (samplePtr == nullptr)
            continue;

        auto& sample = *samplePtr;
        channelProcessor->removeSample(sample);
        urls.push_back(sample.getFileURL());
    }

    soundboards.erase(soundboards.begin() + index);

#if JUCE_ANDROID
    for (const auto& url : urls) {
        if ( ! isSampleURLInUse(url)) {
            AndroidDocumentPermission::releasePersistentReadWriteAccess(url);
        }
    }
#endif

    
    if (selectedSoundboardIndex.has_value()) {
        auto selected = *selectedSoundboardIndex;

        if (soundboards.empty()) {
            selectedSoundboardIndex = std::nullopt;
        }
        else if (selected == index) {
            selectedSoundboardIndex = jmin(index, static_cast<int>(soundboards.size()) - 1);
        }
        else if (selected > index) {
            selectedSoundboardIndex = selected - 1;
        }
    }

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::selectSoundboard(int index)
{
    if (getNumberOfSoundboards() == 0) {
        selectedSoundboardIndex = {};
    }
    else {
        selectedSoundboardIndex = jlimit(0, static_cast<int>(getNumberOfSoundboards()) - 1, index);
    }

    saveToDisk();
}

void SoundboardProcessor::reorderSoundboards()
{
    if (soundboards.empty()) {
        selectedSoundboardIndex = std::nullopt;
        return;
    }

    // Figure out what the new (sorted) indices will be.
    auto originalSelectedIndex = selectedSoundboardIndex.value_or(-1);
    auto originalIndices = sortIndexPreview(soundboards);

    // Determine new indices of the selected soundboard.
    if (originalSelectedIndex < 0) {
        selectedSoundboardIndex = { 0 };
    }
    else {
        auto iterator = std::find(originalIndices.begin(), originalIndices.end(), originalSelectedIndex);
        selectedSoundboardIndex = iterator != originalIndices.end()
                                   ? std::optional<int>(static_cast<int>(std::distance(originalIndices.begin(), iterator)))
                                   : std::optional<int>(0);
    }

    // Above was just a sort preview and logic on that. End with actually sorting the list of soundboards.
    std::sort(soundboards.begin(), soundboards.end(), [](const Soundboard& a, const Soundboard& b) {
        return a.getName() < b.getName();
    });
}

SoundSample* SoundboardProcessor::addSoundSample(String name, String absolutePath, std::optional<int> index)
{
    // Per definition: do nothing when no soundboard is selected or specified.
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return nullptr;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return nullptr;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    sampleList.emplace_back(std::make_unique<SoundSample>(std::move(name), URL(File(absolutePath))));

    saveToDisk();

    return sampleList.back().get();
}

bool SoundboardProcessor::moveSoundSample(int fromSampleIndex, int toSampleIndex, std::optional<int> index)
{
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return false;
    }
    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return false;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    if (fromSampleIndex < 0 || fromSampleIndex >= static_cast<int>(sampleList.size())
        || toSampleIndex < 0 || toSampleIndex > static_cast<int>(sampleList.size())) {
        return false;
    }

    auto movedSample = std::move(sampleList[static_cast<size_t>(fromSampleIndex)]);
    sampleList.erase(sampleList.begin() + fromSampleIndex);

    if (toSampleIndex > fromSampleIndex)
        --toSampleIndex;

    toSampleIndex = jlimit(0, static_cast<int>(sampleList.size()), toSampleIndex);
    sampleList.insert(sampleList.begin() + toSampleIndex, std::move(movedSample));
    
    saveToDisk();

    return true;
}


void SoundboardProcessor::editSoundSample(SoundSample& sampleToUpdate, bool saveIt)
{
#if JUCE_ANDROID
    AndroidDocumentPermission::takePersistentReadOnlyAccess(sampleToUpdate.getFileURL());
#endif

    if (saveIt) {
        saveToDisk();
    }

    updatePlaybackSettings(sampleToUpdate);
}

void SoundboardProcessor::updatePlaybackSettings(SoundSample& sampleToUpdate)
{
    // Immediately update transport source with new playback settings when this sample is currently playing
    auto& activeSamples = channelProcessor->getActiveSamples();
    auto playbackManager = activeSamples.find(&sampleToUpdate);
    if (playbackManager != activeSamples.end()) {
        playbackManager->second->reloadPlaybackSettingsFromSample();
    }
}

bool SoundboardProcessor::containsSample(const SoundSample* sample)
{
    if (sample == nullptr)
        return false;

    for (auto& soundboard : soundboards) {
        for (auto& ownedSample : soundboard.getSamples()) {
            if (ownedSample.get() == sample)
                return true;
        }
    }

    return false;
}

bool SoundboardProcessor::isSampleURLInUse(const juce::URL & url)
{
    for (auto& soundboard : soundboards) {
        auto& sampleList = soundboard.getSamples();
        
        for (auto& samplePtr : sampleList) {
            if (samplePtr != nullptr && samplePtr->getFileURL() == url) {
                return true;
            }
        }
    }
    return false;
}

bool SoundboardProcessor::deleteSoundSample(SoundSample& sampleToDelete, std::optional<int> index)
{
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return false;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return false;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    for (auto iter = sampleList.begin(); iter != sampleList.end(); ++iter) {
        if (*iter == nullptr)
            continue;

        auto& sample = **iter;
        if (&sample == &sampleToDelete) {
            channelProcessor->removeSample(sample);

            auto url = sample.getFileURL();
            
            sampleList.erase(iter);

#if JUCE_ANDROID
            if ( ! isSampleURLInUse(url)) {
                AndroidDocumentPermission::releasePersistentReadWriteAccess(url);
            }
#endif

            break;
        }
    }

    saveToDisk();
    return true;
}

void SoundboardProcessor::stopAllPlayback()
{
    channelProcessor->unloadAll();
}

void SoundboardProcessor::onPlaybackFinished(SamplePlaybackManager* playbackManager)
{
    if (auto * sample = playbackManager->getSample()) {
        if (sample->getEndPlaybackBehaviour() == SoundSample::EndPlaybackBehaviour::NEXT_AT_END) {
            // trigger the next one in the relevant soundboard
            for (auto& soundboard : soundboards) {
                auto& sampleList = soundboard.getSamples();
                
                bool playit = false;
                bool foundboard = false;
                for (auto& sampPtr : sampleList) {
                    if (sampPtr == nullptr)
                        continue;

                    auto& samp = *sampPtr;
                    if (playit) {
                        // we are the next, trigger playback
                        DBG("Triggering next sample");
                        auto playbackManagerMaybe = channelProcessor->loadSample(samp);
                        if (playbackManagerMaybe.has_value()) {
                            playbackManagerMaybe->get()->attach(this);
                            playbackManagerMaybe->get()->play();
                            if (onPlaybackStateChange) {
                                onPlaybackStateChange();
                            }
                        }
                        break;
                    }

                    if (foundboard) break;
                    
                    if (&samp == sample) {
                        playit = true;
                        foundboard = true;
                        continue;
                    }
                }
            }
        }
    }
}



void SoundboardProcessor::writeSoundboardsToFile(const File& file)
{
    ValueTree tree(SOUNDBOARDS_KEY);

    tree.setProperty(SELECTED_KEY, selectedSoundboardIndex.value_or(-1), nullptr);
    tree.setProperty(HOTKEYS_MUTED_KEY, hotkeysMuted, nullptr);
    tree.setProperty(HOTKEYS_NUMERIC_KEY, numericHotkeyAllowed, nullptr);

    int i = 0;
    for (auto& soundboard: soundboards) {
        tree.addChild(soundboard.serialize(), i++, nullptr);
    }

    // Make sure  the parent directory exists
    file.getParentDirectory().createDirectory();

    tree.createXml()->writeTo(file);
}

void SoundboardProcessor::readSoundboardsFromFile(const File& file)
{
    if (!file.existsAsFile()) {
        return;
    }

    XmlDocument doc(file);
    auto xml = doc.getDocumentElement();
    if (xml == nullptr)
        return;

    auto tree = ValueTree::fromXml(*xml);
    if (! tree.isValid())
        return;

    int selected = tree.getProperty(SELECTED_KEY);
    selectedSoundboardIndex = selected >= 0 ? std::optional<int>(selected) : std::nullopt;
    hotkeysMuted = tree.getProperty(HOTKEYS_MUTED_KEY, hotkeysMuted);
    numericHotkeyAllowed = tree.getProperty(HOTKEYS_NUMERIC_KEY, numericHotkeyAllowed);

    soundboards.clear();

    for (const auto& child: tree) {
        soundboards.emplace_back(Soundboard::deserialize(child));
    }
}

void SoundboardProcessor::saveToDisk()
{
    writeSoundboardsToFile(soundboardsFile);
}

void SoundboardProcessor::loadFromDisk()
{
    readSoundboardsFromFile(soundboardsFile);
    reorderSoundboards();
}
