#!/usr/bin/env bash

ROOT="src"
EXCLUDE=("")
EXTENSIONS=("*.cpp" "*.hpp")
CLANG_FORMAT="clang-format"

if ! root_path=$(realpath "$ROOT" 2>/dev/null); then
  echo "Root path '$ROOT' not found." >&2
  exit 2
fi

exclude_norm=()
for e in "${EXCLUDE[@]}"; do
  # Normalize excludes to forward-slash paths so they match full_norm.
  e_norm="${e//\\//}"
  e_norm=$(printf '%s' "$e_norm" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//' | tr '[:upper:]' '[:lower:]')
  if [ -n "$e_norm" ]; then
    exclude_norm+=("$e_norm")
  fi
done

if ! cf_path=$(command -v "$CLANG_FORMAT" 2>/dev/null); then
  echo "clang-format not found in PATH as '$CLANG_FORMAT'. Please install clang-format or update the script to point to the executable." >&2
  exit 3
fi

files=()
while IFS= read -r -d '' f; do
  files+=("$f")
done < <(find "$root_path" -type f \( -name "${EXTENSIONS[0]}" -o -name "${EXTENSIONS[1]}" \) -print0 2>/dev/null)

if [ ${#files[@]} -eq 0 ]; then
  echo "No files found under $root_path matching patterns: ${EXTENSIONS[0]}, ${EXTENSIONS[1]}"
  exit 0
fi

files_to_format=()
for f in "${files[@]}"; do
  full="$f"
  full_norm=$(printf '%s' "$full" | tr '[:upper:]' '[:lower:]' | sed 's|\\|/|g')
  skip=false

  for e in "${exclude_norm[@]}"; do
    # echo "Checking if '$full_norm' contains exclude pattern '$e'"
    if [[ "$full_norm" == *"$e"* ]]; then
      # echo "Excluding $full due to pattern '$e'"
      skip=true
      break
    fi
  done

  if [ "$skip" = false ]; then
    files_to_format+=("$full")
  fi
done

if [ ${#files_to_format[@]} -eq 0 ]; then
  echo "No files to format after applying excludes."
  exit 0
fi

echo "Found ${#files_to_format[@]} file(s) to format."

errors=()
for f in "${files_to_format[@]}"; do
  # echo "Formatting $f"
  if ! "$cf_path" -i "$f"; then
    errors+=("$f")
    echo "Failed to format $f" >&2
  fi
done

if [ ${#errors[@]} -gt 0 ]; then
  echo "Formatting failed for ${#errors[@]} file(s)." >&2
  exit 1
else
  echo "Formatting complete."
  exit 0
fi