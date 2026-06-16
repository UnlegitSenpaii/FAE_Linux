#!/bin/bash

# Config

# prompt | keep | auto-update
FAE_UPDATE_MODE="keep"

# 0: spawn terminal when needed, 1: always headless
FAE_HEADLESS="1"

FACTORIO_BIN_SUBDIR="bin/x64"
FACTORIO_BIN_NAME="factorio"
FACTORIO_PATCHED_NAME="factorio_patched"
FAE_LINUX_BIN_NAME="FAE_Linux"
FAE_LINUX_BUILD_DIR_NAME=".fae_linux_build"
FAE_LINUX_REPO_URL="https://github.com/UnlegitSenpaii/FAE_Linux.git"
FAE_LINUX_BRANCH="master"

# Relaunch inside a terminal when possible.

if [ "${FAE_HEADLESS:-0}" = "1" ]; then
    export FAE_IN_TERMINAL=1
fi

if [ ! -t 1 ] && [ -z "${FAE_IN_TERMINAL:-}" ]; then
    SELF="$(readlink -f "${BASH_SOURCE[0]}")"

    LAUNCHER_TMP="$(mktemp /tmp/fae_launch_XXXXXX.sh 2>/dev/null)" || {
        echo "[WARNING] Could not create temp launcher file; running without a terminal window." >&2
        LAUNCHER_TMP=""
    }
    if [ -n "$LAUNCHER_TMP" ]; then
        {
            printf '#!/bin/bash\n'
            printf 'export FAE_IN_TERMINAL=1\n'
            printf 'bash %q' "$SELF"
            printf ' %q' "$@"
            printf '\n'
            printf 'EXIT_CODE=$?\n'
            printf 'rm -f %q\n' "$LAUNCHER_TMP"
            printf 'if [ $EXIT_CODE -ne 0 ]; then\n'
            printf '    echo\n'
            printf '    read -rp "Press Enter to close..."\n'
            printf 'fi\n'
            printf 'exit $EXIT_CODE\n'
        } > "$LAUNCHER_TMP"
        chmod +x "$LAUNCHER_TMP"

        _exec_in_term() {
            command -v "$1" &>/dev/null || return 1
            local _rc
            case "$1" in
                gnome-terminal) gnome-terminal --wait -- bash "$LAUNCHER_TMP"; _rc=$? ;;
                kitty|foot)     "$1" bash "$LAUNCHER_TMP";                     _rc=$? ;;
                wezterm)        wezterm start bash "$LAUNCHER_TMP";             _rc=$? ;;
                *)              "$1" -e bash "$LAUNCHER_TMP";                  _rc=$? ;;
            esac
            if [ "$_rc" -eq 0 ]; then
                rm -f "$LAUNCHER_TMP"
                exit 0
            fi
            echo "[WARNING] Terminal \"$1\" exited with code $_rc; trying next emulator." >&2
            return 1
        }
        _exec_in_term x-terminal-emulator
        [ -n "${TERMINAL:-}" ] && _exec_in_term "${TERMINAL##*/}"
        for _t in gnome-terminal konsole xfce4-terminal alacritty kitty wezterm foot xterm urxvt tilix terminator; do
            _exec_in_term "$_t"
        done
        unset -f _exec_in_term

        rm -f "$LAUNCHER_TMP"
        echo "[WARNING] No terminal emulator found or all failed to launch. Falling back to headless mode." >&2
    fi
fi

#set -euo pipefail

# Steam libs can break build tools; strip for build, restore for launch.
STEAM_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
STEAM_LD_PRELOAD="${LD_PRELOAD:-}"

run_clean() {
    env -u LD_LIBRARY_PATH -u LD_PRELOAD "$@"
}

# Returns 0 if all build deps exist.
check_build_deps() {
    local missing=()
    command -v git   &>/dev/null || missing+=("git")
    command -v cmake &>/dev/null || missing+=("cmake")
    { command -v g++ &>/dev/null || command -v clang++ &>/dev/null; } \
        || missing+=("g++ or clang++")
    if [ ${#missing[@]} -gt 0 ]; then
        echo "[WARNING] Missing build dependencies: ${missing[*]}"
        return 1
    fi
    return 0
}

# download_latest_release <destination_path>
download_latest_release() {
    local dest="$1"
    local download_url="https://github.com/UnlegitSenpaii/FAE_Linux/releases/latest/download/FAE_Linux"

    local dl_cmd=""
    if   command -v curl &>/dev/null; then dl_cmd="curl"
    elif command -v wget &>/dev/null; then dl_cmd="wget"
    else
        echo "[ERROR] Neither curl nor wget is available. Cannot download FAE_Linux."
        return 1
    fi

    local dest_tmp
    dest_tmp="$(mktemp "${dest}.XXXXXX")" || {
        echo "[ERROR] Could not create temporary file for download."
        return 1
    }

    echo "[INFO] Downloading FAE_Linux from: $download_url"
    if [ "$dl_cmd" = "curl" ]; then
        run_clean curl -fsSL -o "$dest_tmp" "$download_url" \
            || { rm -f "$dest_tmp"; echo "[ERROR] Download failed. Check your network connection and that a release exists."; return 1; }
    else
        run_clean wget -qO "$dest_tmp" "$download_url" \
            || { rm -f "$dest_tmp"; echo "[ERROR] Download failed. Check your network connection and that a release exists."; return 1; }
    fi

    if [ ! -s "$dest_tmp" ]; then
        rm -f "$dest_tmp"
        echo "[ERROR] Downloaded file is empty; the release may not have a binary asset yet."
        return 1
    fi

    if command -v file &>/dev/null; then
        local arch file_out arch_ok
        arch="$(uname -m)"
        file_out="$(run_clean file "$dest_tmp" 2>/dev/null)"
        arch_ok=true
        case "$arch" in
            x86_64)  printf '%s' "$file_out" | grep -qE 'x86-64|x86_64'      || arch_ok=false ;;
            aarch64) printf '%s' "$file_out" | grep -qE 'aarch64|ARM aarch64' || arch_ok=false ;;
        esac
        if [ "$arch_ok" = false ]; then
            rm -f "$dest_tmp"
            echo "[ERROR] Downloaded binary architecture does not match this system ($arch)."
            echo "[ERROR] file output: $file_out"
            return 1
        fi
    fi

    mv "$dest_tmp" "$dest"
    chmod +x "$dest"
    echo "[SUCCESS] FAE_Linux downloaded to: $dest"
}

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FACTORIO_BIN_DIR="$SCRIPT_DIR/$FACTORIO_BIN_SUBDIR"
FACTORIO_BIN="$FACTORIO_BIN_DIR/$FACTORIO_BIN_NAME"
FACTORIO_PATCHED_BIN="$FACTORIO_BIN_DIR/$FACTORIO_PATCHED_NAME"
FAE_LINUX_BIN="$SCRIPT_DIR/$FAE_LINUX_BIN_NAME"
FAE_LINUX_REPO_TMP="$SCRIPT_DIR/$FAE_LINUX_BUILD_DIR_NAME"

if [ ! -f "$FACTORIO_BIN" ]; then
    echo "[ERROR] Factorio binary not found at: $FACTORIO_BIN"
    echo "[ERROR] Make sure this script is placed in the Factorio root directory."
    exit 1
fi

# get BuildID[sha1] from binary
get_build_id() {
    local binary="$1"
    if command -v file &>/dev/null; then
        run_clean file "$binary" 2>/dev/null \
            | sed -n 's/.*BuildID\[sha1\]=\([0-9a-f]*\).*/\1/p'
    elif command -v readelf &>/dev/null; then
        readelf -n "$binary" 2>/dev/null \
            | sed -n 's/.*Build ID: \([0-9a-f]*\).*/\1/p' | head -1
    fi
}

ORIGINAL_BUILD_ID="$(get_build_id "$FACTORIO_BIN")"
if [ -z "$ORIGINAL_BUILD_ID" ]; then
    echo "[WARNING] Could not determine Factorio BuildID ('file'/'readelf' unavailable)."
    echo "[WARNING] Up-to-date check disabled; will always re-patch when needed."
fi
echo "[INFO] Factorio BuildID[sha1]: ${ORIGINAL_BUILD_ID:-(unknown)}"

needs_patch=true

if [ -f "$FACTORIO_PATCHED_BIN" ] && [ -f "$FAE_LINUX_BIN" ] && [ -n "$ORIGINAL_BUILD_ID" ]; then
    PATCHED_BUILD_ID="$(get_build_id "$FACTORIO_PATCHED_BIN")"
    if [ -n "$PATCHED_BUILD_ID" ] && [ "$PATCHED_BUILD_ID" = "$ORIGINAL_BUILD_ID" ]; then
        echo "[SUCCESS] Patched binary is up-to-date (BuildID matches). Launching..."
        needs_patch=false
    else
        echo "[INFO] BuildID mismatch; Factorio was updated, re-patching is needed."
    fi
else
    echo "[INFO] Patched binary or FAE_Linux patcher not found."
fi

if [ "$needs_patch" = true ]; then

    needs_rebuild=false

    case "${FAE_UPDATE_MODE,,}" in
        keep)
            if [ ! -f "$FAE_LINUX_BIN" ]; then
                echo "[ERROR] FAE_UPDATE_MODE=keep but no FAE_Linux binary found at: $FAE_LINUX_BIN"
                echo "[ERROR] Place a pre-compiled binary there or change FAE_UPDATE_MODE to \"prompt\" or \"auto-update\"."
                exit 1
            fi
            echo "[INFO] FAE_UPDATE_MODE=keep; using existing FAE_Linux binary."
            ;;
        auto-update)
            echo "[INFO] FAE_UPDATE_MODE=auto-update; pulling latest from GitHub."
            needs_rebuild=true
            ;;
        *) # prompt (default)
            if [ -f "$FAE_LINUX_BIN" ]; then
                echo
                echo "[WARNING] Existing FAE_Linux patcher found at: $FAE_LINUX_BIN"
                echo "[WARNING] Verify repo before updating: $FAE_LINUX_REPO_URL"
                echo
                if [ -t 0 ]; then
                    read -rp "  [e] Use existing patcher   [u] Update from GitHub   (e/u, default: e): " UPDATE_CHOICE
                else
                    UPDATE_CHOICE="e"
                fi
                case "${UPDATE_CHOICE,,}" in
                    e|existing) echo "[INFO] Using existing FAE_Linux binary." ;;
                    *)          needs_rebuild=true ;;
                esac
            else
                echo
                echo "[WARNING] FAE_Linux patcher not found; it needs to be built from source."
                echo "[WARNING] Source: $FAE_LINUX_REPO_URL (branch: $FAE_LINUX_BRANCH)"
                echo
                if [ -t 0 ]; then
                    read -rp "  Proceed with build from GitHub? [Y/n]: " BUILD_CHOICE
                    case "${BUILD_CHOICE,,}" in
                        n|no)
                            echo "[ERROR] Build cancelled."
                            echo "[ERROR] To use an existing binary place it at: $FAE_LINUX_BIN"
                            exit 1
                            ;;
                        *) needs_rebuild=true ;;
                    esac
                else
                    echo "[INFO] Headless mode: no binary found; downloading pre-built release binary..."
                    download_latest_release "$FAE_LINUX_BIN" || {
                        echo "[ERROR] Could not obtain FAE_Linux binary automatically."
                        echo "[ERROR] Set FAE_UPDATE_MODE=\"auto-update\" or place a pre-compiled binary at:"
                        echo "[ERROR]   $FAE_LINUX_BIN"
                        exit 1
                    }
                fi
            fi
            ;;
    esac

    if [ "$needs_rebuild" = true ]; then
        if check_build_deps; then
            echo "[INFO] Cloning FAE_Linux $FAE_LINUX_BRANCH branch..."

            rm -rf "$FAE_LINUX_REPO_TMP"
            mkdir -p "$FAE_LINUX_REPO_TMP"

            run_clean git clone --depth 1 -b "$FAE_LINUX_BRANCH" \
                "$FAE_LINUX_REPO_URL" \
                "$FAE_LINUX_REPO_TMP" || {
                echo "[ERROR] Failed to clone FAE_Linux repository."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            echo "[INFO] Building FAE_Linux..."
            BUILD_DIR="$FAE_LINUX_REPO_TMP/cmake_build"
            mkdir -p "$BUILD_DIR"

            run_clean cmake -S "$FAE_LINUX_REPO_TMP" \
                  -B "$BUILD_DIR" \
                  -DCMAKE_BUILD_TYPE=Release || {
                echo "[ERROR] cmake configuration failed."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            run_clean cmake --build "$BUILD_DIR" --config Release -- -j"$(nproc 2>/dev/null || echo 1)" || {
                echo "[ERROR] Build failed."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            BUILT_BIN="$(find "$BUILD_DIR/out/bin" -name "FAE_Linux" -type f 2>/dev/null | head -1)"
            if [ -z "$BUILT_BIN" ]; then
                echo "[ERROR] Could not locate built FAE_Linux binary."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            fi

            cp "$BUILT_BIN" "$FAE_LINUX_BIN"
            chmod +x "$FAE_LINUX_BIN"
            rm -rf "$FAE_LINUX_REPO_TMP"
            echo "[SUCCESS] FAE_Linux built and installed to: $FAE_LINUX_BIN"
        else
            echo "[WARNING] One or more build dependencies are missing (see above)."
            echo "[WARNING] Falling back to downloading the latest pre-built release binary."
            download_latest_release "$FAE_LINUX_BIN" || {
                echo "[ERROR] Could not obtain FAE_Linux binary. Aborting."
                exit 1
            }
        fi
    fi

    echo "[INFO] Copying factorio -> factorio_patched..."
    cp "$FACTORIO_BIN" "$FACTORIO_PATCHED_BIN"

    echo "[INFO] Running FAE_Linux patcher on factorio_patched..."
    FAE_EXTRA_ARGS=()
    if [ ! -t 1 ]; then
        FAE_EXTRA_ARGS+=("--no-prompt")
    fi
    "$FAE_LINUX_BIN" "$FACTORIO_PATCHED_BIN" "${FAE_EXTRA_ARGS[@]+"${FAE_EXTRA_ARGS[@]}"}" || {
        PATCHER_EXIT=$?
        echo "[ERROR] FAE_Linux patcher exited with code $PATCHER_EXIT"
        rm -f "$FACTORIO_PATCHED_BIN"
        exit $PATCHER_EXIT
    }

    echo "[SUCCESS] Patching completed successfully!"
fi

echo "[INFO] Launching factorio_patched..."

export LD_LIBRARY_PATH="$STEAM_LD_LIBRARY_PATH"
export LD_PRELOAD="$STEAM_LD_PRELOAD"

LAUNCH_CMD=()
USED_STEAM_CHAIN=false
for arg in "$@"; do
    if [ "$arg" = "$FACTORIO_BIN" ]; then
        LAUNCH_CMD+=("$FACTORIO_PATCHED_BIN")
        USED_STEAM_CHAIN=true
    else
        LAUNCH_CMD+=("$arg")
    fi
done

if [ "$USED_STEAM_CHAIN" = false ]; then
    LAUNCH_CMD=("$FACTORIO_PATCHED_BIN" "$@")
fi

echo "[SUCCESS] Factorio launched. Handing control to Steam..."
exec "${LAUNCH_CMD[@]}"