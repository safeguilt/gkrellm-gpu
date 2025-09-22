# Developer Notes

## Building
The `Makefile` compiles sources from `src/` into `build/`. Run `make clean` to remove artefacts. All compiler options are derived via `pkg-config` to keep the build portable.

## Code Layout
- `gkrellm_init_plugin` registers the monitor and initialises NVML on first use.
- `update_gpu` executes once per second, collecting utilisation and memory metrics.
- `format_gpu_text` assembles the overlay string, respecting GKrellM escape sequences.

## Testing
- Start GKrellM with the compiled plugin to verify rendering.
- Compare generated data with `nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total --format=csv`
- Use `valgrind --tool=memcheck gkrellm` during development to detect leaks (NVML handles cleanly shutdown on exit).

## Releasing
1. Update `CHANGELOG.md` with the new version.
2. Tag the release (e.g. `git tag -a v0.1.0 -m "First public release"`).
3. Build the plugin (`make clean && make`).
4. Attach `build/gkrellm-gpu.so` and the changelog to the GitHub Release page.
