#!/usr/bin/env bash
# Store the xz-compressed ROM in repository Actions secrets.
#
# GitHub caps a single secret at 48 KB, so the base64 stream is split into
# 45,000-character chunks named ROM_CHUNK_001, ROM_CHUNK_002, ... The workflow
# concatenates the chunks, base64-decodes and xz-decompresses them in the
# runner, then checks the SHA-256 before building. The ROM is never committed.
#
# Usage: tools/ci/upload-rom-secrets.sh ["James Pond 3 (USA, Europe).md.xz"]
# Requires: gh (logged in with repo admin), xz, base64, split.
set -euo pipefail

cd "$(dirname "$0")/../.."
rom=${1:-"James Pond 3 (USA, Europe).md.xz"}
rom_size=2097152
rom_sha=8f52e27945b96d6c37fda28c44d39d65d805cc0b58332f9b4a2eb26dff540d29
chunk_chars=45000

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ ! -f "$rom" ]; then
    echo "missing ROM archive: $rom" >&2
    exit 1
fi

# Refuse to upload anything that is not the expected dump.
xz -dc -- "$rom" > "$tmp/jp3.md"
actual_sha=$(sha256sum "$tmp/jp3.md" | cut -d' ' -f1)
actual_size=$(stat -c %s "$tmp/jp3.md")
if [ "$actual_size" -ne "$rom_size" ] || [ "$actual_sha" != "$rom_sha" ]; then
    echo "not the expected ROM: $actual_size bytes, sha256 $actual_sha" >&2
    exit 1
fi

base64 -w0 -- "$rom" > "$tmp/rom.b64"
split -b "$chunk_chars" -d -a 3 --additional-suffix=.chunk "$tmp/rom.b64" "$tmp/part."

count=$(find "$tmp" -name 'part.*.chunk' | wc -l)
slots=$(grep -c 'secrets\.ROM_CHUNK_' .github/workflows/build.yml)
if [ "$count" -gt "$slots" ]; then
    echo "$count chunks are needed but the workflow has $slots slots" >&2
    exit 1
fi

# Remove stale chunks first: an old, longer ROM must not be appended to.
while IFS= read -r name; do
    [ -n "$name" ] && gh secret delete "$name"
done < <(gh secret list --json name --jq '.[].name' | grep '^ROM_CHUNK_' || true)

i=0
for f in "$tmp"/part.*.chunk; do
    i=$((i + 1))
    gh secret set "$(printf 'ROM_CHUNK_%03d' "$i")" < "$f"
done

printf 'Uploaded %d chunks (%d bytes compressed, %d bytes ROM).\n' \
    "$i" "$(stat -c %s "$rom")" "$actual_size"
