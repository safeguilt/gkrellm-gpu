#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-$(date +%Y.%m.%d)}"
ARCHIVE="gkrellm-gpu-plugin-${VERSION}.tar.gz"

# Ensure a clean build artefact
make clean >/dev/null 2>&1 || true
make

tar -czf "$ARCHIVE" \
  LICENSE \
  README.md \
  CHANGELOG.md \
  docs \
  assets \
  build/gkrellm-gpu.so

echo "Created $ARCHIVE"
