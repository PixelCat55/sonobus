# Fork Changes

This repository is an **unofficial fork of SonoBus** (`sonosaurus/sonobus`).

It does **not** claim ownership of the original SonoBus project, branding, design, source history, or upstream contributions. The purpose of this fork is to maintain clearly documented quality-of-life improvements, bug fixes, and conservative background-efficiency work on top of the original project.

SonoBus remains licensed under the **GNU General Public License v3.0 (GPLv3)**, together with the repository's existing license exception where applicable. Original copyright and license notices are preserved.

## Fork goals

The fork is intended to stay close to upstream SonoBus while adding practical QoL improvements, bug fixes, and lightweight background behavior for Windows and Linux desktop users. Changes should be:

- small, understandable, and testable;
- optional where possible;
- implemented directly in source rather than through external helper processes;
- documented transparently;
- avoid unnecessary background UI work while preserving real-time audio/network behavior;
- easy to distinguish from upstream SonoBus behavior.

## Current fork-specific modifications

### 1. Native Windows minimize-to-tray

**Added:** 2026-08-17

The Windows standalone SonoBus application now integrates directly with the Windows notification area.

Behavior:

- Clicking the normal Windows **Minimize** button hides the SonoBus main window from the taskbar.
- SonoBus continues running normally in the background.
- Audio and network processing remain active while the window is hidden.
- A SonoBus tray icon remains available in the Windows notification area.
- Left-clicking the tray icon restores the SonoBus window.
- Right-clicking the tray icon opens a context menu containing:
  - **Open SonoBus**
  - **Exit SonoBus**
- The normal window **Close (X)** button keeps its original behavior and exits SonoBus.

Implementation notes:

- The feature is native to the standalone application source.
- No sidecar tray program or helper process is used.
- The minimize handling is deferred through JUCE's message loop to avoid racing the native Windows minimize transition and leaving stale taskbar state.

### 2. Start with Windows, minimized to tray

**Added:** 2026-08-17

The Windows standalone application's **Settings → OPTIONS** tab now includes:

**Start with Windows (minimized to tray)**

Behavior:

- Enabling the toggle registers SonoBus to start for the **current Windows user** at sign-in.
- It uses the standard per-user Windows `Run` registry location under `HKEY_CURRENT_USER`.
- Administrator privileges are not required.
- The startup entry launches the current `SonoBus.exe` with the internal `--start-minimized` argument.
- At sign-in, SonoBus initializes normally but keeps the main window hidden and remains available from the system tray.
- Disabling the toggle removes the startup entry.
- The in-app toggle reads the actual registry state when the Options panel is opened or refreshed.

Path behavior:

The startup entry stores the exact executable path that enabled the option. If the user later moves or renames the executable, they should disable and re-enable the setting from the new location.

Runtime status:

This behavior has been compiled successfully through the repository's Windows GitHub Actions build and confirmed working in normal Windows runtime testing by the fork maintainer.

### 3. Stability and background-efficiency hardening

**Started:** 2026-08-31

The QoL and Bug Fixes branch begins a conservative hardening pass based on a full-source audit.

Initial changes:

- soundboard sample storage keeps sample addresses stable when new samples are appended, preventing active playback managers from being left with invalid pointers after container growth;
- delayed GUI callbacks in several frequently used views use JUCE safe pointers instead of queued raw component pointers;
- the main editor skips peer/layout/status UI polling while the complete window is hidden in the Windows tray, while audio and network processing continue unchanged;
- Windows screen-saver suppression is disabled while SonoBus is tray-hidden so a background instance does not keep Windows awake unnecessarily;
- redundant per-peer audio buffer work was removed and peer work buffers are preallocated during configuration to reduce real-time allocation pressure;
- the inherited beta-version comparison bug is corrected;
- the Windows CI scope is expanded for the QoL and Bug Fixes development work.

Per maintainer direction, this hardening pass deliberately does **not** change SonoBus password handling/storage or transport encryption behavior.

### 4. Linux Mint tray, autostart, and reproducible build

**Added:** 2026-09-02

The Linux standalone application now includes the fork's native tray-hidden operation and `--start-minimized` support. Its **Settings → OPTIONS** panel provides **Start with Linux (minimized to tray)**, implemented with a per-user freedesktop autostart entry under `~/.config/autostart`.

The Linux implementation:

- uses the same in-process JUCE tray component as the Windows version;
- keeps audio and networking active while the main window is hidden;
- restores screen-saver behavior while tray-hidden;
- requires no root access, service, scheduled task, or helper process;
- stores the exact executable path in a fork-specific per-user desktop entry;
- preserves the explicit constraints against password, encryption, and transport changes.

An Ubuntu 22.04 x64 GitHub Actions workflow builds and smoke-tests a compressed standalone package intended for Linux Mint 21.x and newer x64 installations.

## Build integration

The fork-specific Windows and Linux changes are part of the normal source tree. There is **no patch script** or post-checkout transformation required.

The repository includes Windows x64 and Linux x64 GitHub Actions workflows that:

- check out the requested revision;
- provision the platform build dependencies;
- verify the tray and autostart source integration;
- configure the project with CMake;
- builds the `SonoBus_Standalone` target;
- upload the resulting platform test package as an artifact.

For manual build information, see [`BUILD_WINDOWS.md`](BUILD_WINDOWS.md) and [`BUILD_LINUX.md`](BUILD_LINUX.md).

## Upstream relationship

Original project: **SonoBus** by Jesse Chappell / Sonosaurus and the upstream contributors.

Original repository:

`https://github.com/sonosaurus/sonobus`

This repository:

`https://github.com/PixelCat55/sonobus`

This fork is **not an official SonoBus release**. General SonoBus bugs and upstream behavior belong to the original project; issues caused specifically by the modifications documented here belong to this fork.

## Ownership and attribution

The maintainer of this fork claims ownership only over new fork-specific contributions to the extent allowed by the applicable license. No ownership is claimed over the original SonoBus project or upstream work.

The intent is to keep attribution straightforward: **SonoBus is SonoBus; this fork simply adds optional convenience features on top of it.**

## License

This fork is distributed under the same applicable open-source licensing terms as upstream SonoBus. See:

- [`LICENSE`](LICENSE)
- [`LICENSE_EXCEPTION`](LICENSE_EXCEPTION)

Third-party dependencies retain their own copyright and license terms.
