#!/usr/bin/env bash
set -euo pipefail

# Simple wrapper that ensures the project builds using the local toolchain.
make "$@"
