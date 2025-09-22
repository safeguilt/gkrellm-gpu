# Installation Guide

## Prerequisites
- GKrellM 2.x and matching development headers (`gkrellm-devel` or `gkrellm-dev`).
- GTK+ 2.0 development packages (usually installed alongside GKrellM headers).
- Proprietary NVIDIA driver with the NVIDIA Management Library `libnvidia-ml.so`.
- `pkg-config`, `make`, and a C compiler (`gcc`).

Verify NVML is available:
```bash
nvidia-smi
ldconfig -p | grep nvidia-ml
```

## Build Steps
```bash
git clone git@github.com:safeguilt/gkrellm-gpu.git
cd gkrellm-gpu
make
```

The shared object is generated at `build/gkrellm-gpu.so`.

## Installing the Plugin
```bash
make install   # copies the .so to ~/.gkrellm2/plugins/
```

Alternatively copy manually:
```bash
mkdir -p ~/.gkrellm2/plugins
cp build/gkrellm-gpu.so ~/.gkrellm2/plugins/
```

Restart GKrellM to load the new monitor. If you are packaging for a distribution, drop the `.so` into GKrellM's global plugin directory (often `/usr/lib/gkrellm2/plugins/`).

## Uninstallation
Remove the shared object from the plugins directory:
```bash
rm ~/.gkrellm2/plugins/gkrellm-gpu.so
```
