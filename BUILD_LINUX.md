# Linux Mint Build and Test Guide

This fork's Linux QoL and Bug Fixes changes are part of the normal source tree. The standalone Linux application includes the same cross-platform stability and efficiency work as the Windows build, plus native minimize-to-tray and per-user autostart support.

## Prebuilt test package

The repository's Linux workflow builds on Ubuntu 22.04 x64. This provides a conservative GNU/Linux baseline intended for Linux Mint 21.x and newer x64 releases.

After extracting the build package:

```bash
tar -xzf SonoBus-QoL-Bug-Fixes-Linux-x64.tar.gz
cd SonoBus-QoL-Bug-Fixes-Linux-x64
chmod +x sonobus
./sonobus
```

The package also contains the standard SonoBus desktop entry and icon for users who want to install menu integration manually.

## Build requirements

On Linux Mint, Ubuntu, or another Debian-based distribution:

```bash
sudo apt update
sudo apt install git build-essential cmake ninja-build pkg-config \
  libasound2-dev libjack-jackd2-dev libopus-dev \
  libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libxcomposite-dev libgl1-mesa-dev libfreetype6-dev
```

## Configure and build

```bash
git clone https://github.com/PixelCat55/sonobus.git
cd sonobus
git switch qol-bugfixes-optimization
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --target SonoBus_Standalone --parallel 2
```

The resulting executable is:

```text
build-linux/SonoBus_artefacts/Release/Standalone/sonobus
```

## Linux tray and autostart behavior

- Minimize hides the standalone window and leaves SonoBus available through the system tray.
- Close keeps the normal SonoBus behavior and exits the application.
- **Settings → OPTIONS → Start with Linux (minimized to tray)** creates a per-user desktop autostart entry.
- No root privileges, system service, or helper process are required.
- The autostart entry points to the exact executable that enabled it. Disable and re-enable the option after moving the binary.

See [`LINUX_FEATURES.md`](LINUX_FEATURES.md) for full behavior and troubleshooting details.

## GitHub Actions

`.github/workflows/build-linux-mint.yml` performs the Linux x64 build automatically. It verifies the Linux tray/autostart integration, compiles `SonoBus_Standalone`, checks dynamic libraries, runs a virtual-display `--version` smoke test, and uploads a compressed test package.
