# Linux QoL and Bug Fixes Features

This document describes the Linux desktop features maintained by the unofficial `PixelCat55/sonobus` fork. They are not part of upstream SonoBus unless separately adopted there.

## Native minimize to system tray

On the Linux standalone application, minimizing SonoBus hides the main window from the normal taskbar or panel while audio and networking continue running.

### Tray controls

- **Left-click:** restore the SonoBus main window.
- **Right-click → Open SonoBus:** restore the main window.
- **Right-click → Exit SonoBus:** request a normal shutdown.

The normal **Close (X)** button remains unchanged and exits SonoBus. Only minimize hides it to the tray.

The tray integration uses JUCE's native Linux system-tray support and does not start a helper process. Linux Mint's Cinnamon desktop supports the legacy tray protocol used by this component.

## Start with Linux, minimized to tray

Open **SonoBus Settings → OPTIONS** and enable:

**Start with Linux (minimized to tray)**

SonoBus creates this per-user autostart file:

```text
~/.config/autostart/sonobus-qol-bug-fixes.desktop
```

The entry launches the current executable with:

```text
--start-minimized
```

This needs no root privileges. Disabling the option deletes only the fork-specific autostart entry.

### Moving the executable

The autostart file contains the exact path of the `sonobus` executable that enabled the option. After moving or renaming the binary, open it from the new location and disable/re-enable the option.

## Background behavior

While tray-hidden, SonoBus keeps audio and networking active but avoids unnecessary hidden-window polling and does not suppress the desktop screen saver. This preserves normal low-latency operation while reducing idle desktop work.

## Scope

These features apply to the standalone Linux application. They do not alter VST3/LV2 plugin behavior, password storage, encryption, or network transport behavior.

## Troubleshooting

### No tray icon appears

Make sure the desktop environment has a system-tray area enabled. On Linux Mint Cinnamon this is normally provided by the panel. Other desktop environments may need a tray/status-notifier extension.

### SonoBus does not start after sign-in

The executable may have moved, or its execute permission may have been removed. Launch it manually, confirm it runs, then disable and re-enable the Linux startup option.

### SonoBus does not launch from an extracted archive

Restore the executable permission and try again:

```bash
chmod +x sonobus
./sonobus
```

If a shared library is missing, install the normal SonoBus runtime dependencies supplied by Linux Mint or build locally using [`BUILD_LINUX.md`](BUILD_LINUX.md).
