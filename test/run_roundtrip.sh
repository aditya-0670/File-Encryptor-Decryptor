#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binary="$repo_root/encrypt_decrypt"

if [[ ! -x "$binary" ]]; then
    echo "Missing executable: $binary"
    echo "Run 'make' first, or use 'make test'."
    exit 1
fi

workdir="$repo_root/test/tmp_roundtrip_$$"
input_dir="$workdir/input"
sample_file="$input_dir/sample.txt"
expected_file="$workdir/expected.txt"

mkdir -p "$input_dir"

printf 'Round trip sample data\nSecond line\n' > "$sample_file"
cp "$sample_file" "$expected_file"

if (cd "$workdir" && "$binary" "$input_dir" rotate >/dev/null 2>&1); then
    echo "Expected invalid action to fail."
    exit 1
fi

if ! cmp -s "$sample_file" "$expected_file"; then
    echo "Invalid action modified the input file."
    exit 1
fi

if (cd "$workdir" && "$binary" "$input_dir" encrypt >/dev/null 2>&1); then
    echo "Expected missing .env password to fail."
    exit 1
fi

if ! cmp -s "$sample_file" "$expected_file"; then
    echo "Missing .env password modified the input file."
    exit 1
fi

printf 'VAULT_PASSWORD=correct horse battery staple\n' > "$workdir/.env"

(cd "$workdir" && "$binary" "$input_dir" ENCRYPT >/dev/null)

if cmp -s "$sample_file" "$expected_file"; then
    echo "Encryption did not change the input file."
    exit 1
fi

(cd "$workdir" && "$binary" "$input_dir" DeCrYpT >/dev/null)

if ! cmp -s "$sample_file" "$expected_file"; then
    echo "Decrypt did not restore the original file."
    exit 1
fi

echo "Round-trip test passed: $workdir"
