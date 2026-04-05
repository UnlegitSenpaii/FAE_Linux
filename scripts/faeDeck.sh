#!/bin/bash

#    ███████╗░█████╗░███████╗  ██╗░░░░░██╗███╗░░██╗██╗░░░██╗██╗░░██╗
#    ██╔════╝██╔══██╗██╔════╝  ██║░░░░░██║████╗░██║██║░░░██║╚██╗██╔╝
#    █████╗░░███████║█████╗░░  ██║░░░░░██║██╔██╗██║██║░░░██║░╚███╔╝░
#    ██╔══╝░░██╔══██║██╔══╝░░  ██║░░░░░██║██║╚████║██║░░░██║░██╔██╗░
#    ██║░░░░░██║░░██║███████╗  ███████╗██║██║░╚███║╚██████╔╝██╔╝╚██╗
#    ╚═╝░░░░░╚═╝░░╚═╝╚══════╝  ╚══════╝╚═╝╚═╝░░╚══╝░╚═════╝░╚═╝░░╚═╝

# Automation script for Factorio Achievement Enabler for Linux -- written with AI assistance
# Preconfigured for the Steam Deck

# 1. Place this script in your Factorio directory. Example:
#   /home/deck/.local/share/Steam/steamapps/common/Factorio/

# 2. Set as Steam launch option (with full path!!). Example:
#   bash /home/deck/.local/share/Steam/steamapps/common/Factorio/faeDeck.sh %command%

# ---------------------------------------------------------------------------
# Configuration — edit these to match your setup
# ---------------------------------------------------------------------------

# Controls how FAE_Linux updates are handled when a new patch is needed.
#   prompt      — ask each time (default, recommended)
#   keep        — always use the existing binary without asking
#   auto-update — always pull and rebuild from GitHub without asking
FAE_UPDATE_MODE="auto-update"

# Set to 1 to skip terminal spawning and run entirely in headless mode.
# Useful on systems without a display server or when you don't want a
# terminal window to appear at all.
#   0 (default) — spawn a terminal window when not already in one
#   1           — always run headless, no terminal window - RECOMMENDED FOR STEAMDECK
FAE_HEADLESS="1"

# Factorio binary subdirectory relative to the Factorio root
FACTORIO_BIN_SUBDIR="bin/x64"

# Name of the original Factorio binary
FACTORIO_BIN_NAME="factorio"

# Name of the patched binary that will be created alongside the original
FACTORIO_PATCHED_NAME="factorio_patched"

# Name of the FAE_Linux patcher binary placed in the Factorio root
FAE_LINUX_BIN_NAME="FAE_Linux"

# Temporary directory name (inside Factorio root) used while building FAE_Linux
FAE_LINUX_BUILD_DIR_NAME=".fae_linux_build"

# FAE_Linux GitHub repository URL and branch to build from
FAE_LINUX_REPO_URL="https://github.com/UnlegitSenpaii/FAE_Linux.git"
FAE_LINUX_BRANCH="master"

# ---------------------------------------------------------------------------
# Terminal window: if not running inside a terminal (e.g. launched by Steam),
# relaunch this script inside one so that FAE_Linux output and prompts are
# visible to the user.
# FAE_IN_TERMINAL guards against infinite relaunching.
# ---------------------------------------------------------------------------

if [ "${FAE_HEADLESS:-0}" = "1" ]; then
    export FAE_IN_TERMINAL=1
fi

if [ ! -t 1 ] && [ -z "${FAE_IN_TERMINAL:-}" ]; then
    SELF="$(readlink -f "${BASH_SOURCE[0]}")"

    # Write a temp launcher script to avoid quoting issues with "$@"
    LAUNCHER_TMP="$(mktemp /tmp/fae_launch_XXXXXX.sh 2>/dev/null)" || {
        printf '[WARNING] Could not create temp launcher file; running without a terminal window.\n' >&2
        LAUNCHER_TMP=""
    }
    if [ -n "$LAUNCHER_TMP" ]; then
        {
            printf '#!/bin/bash\n'
            printf 'export FAE_IN_TERMINAL=1\n'
            # Reconstruct the exact invocation with properly quoted arguments
            printf 'bash %q' "$SELF"
            printf ' %q' "$@"
            printf '\n'
            printf 'EXIT_CODE=$?\n'
            # Clean up this temp file once we are done with it
            printf 'rm -f %q\n' "$LAUNCHER_TMP"
            # Only pause for input if the script exited WITHOUT exec-ing into Factorio
            # (i.e., on an error path). On the happy path exec replaces this process.
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
                # Launcher completed successfully; Factorio is now running detached.
                rm -f "$LAUNCHER_TMP"
                exit 0
            fi
            # Terminal was found but exited with an error — it likely crashed on
            # startup (e.g. no display). Fall through to try the next emulator.
            printf '[WARNING] Terminal "%s" exited with code %d — trying next emulator.\n' "$1" "$_rc" >&2
            return 1
        }
        # 1. Debian/Ubuntu system-configured terminal
        _exec_in_term x-terminal-emulator
        # 2. User's preferred terminal (e.g. export TERMINAL=alacritty in .profile)
        [ -n "${TERMINAL:-}" ] && _exec_in_term "${TERMINAL##*/}"
        # 3. Common fallback list
        for _t in gnome-terminal konsole xfce4-terminal alacritty kitty wezterm foot xterm urxvt tilix terminator; do
            _exec_in_term "$_t"
        done
        unset -f _exec_in_term

        # No terminal emulator could be launched successfully — remove the temp
        # file and continue in headless mode so Factorio still gets patched and
        # started (e.g. Steam Deck where no usable terminal is available).
        rm -f "$LAUNCHER_TMP"
        printf '[WARNING] No terminal emulator found or all failed to launch. Falling back to headless mode.\n' >&2
    fi
fi

#set -euo pipefail

# ---------------------------------------------------------------------------
# Steam injects its own bundled libraries via LD_LIBRARY_PATH and LD_PRELOAD.
# These conflict with system tools (git-remote-https links against system libcurl,
# not Steam's). Save the originals now and strip them for all build commands;
# restore them before launching Factorio so the game still finds Steam's libs.
# ---------------------------------------------------------------------------
STEAM_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
STEAM_LD_PRELOAD="${LD_PRELOAD:-}"

# Wrapper: run a command with Steam's env vars stripped
run_clean() {
    env -u LD_LIBRARY_PATH -u LD_PRELOAD "$@"
}

# ---------------------------------------------------------------------------
# Colors
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }

# ---------------------------------------------------------------------------
# Helper: check that all build-time dependencies are available.
# Returns 0 if everything is present, 1 if anything is missing.
# ---------------------------------------------------------------------------
check_build_deps() {
    local missing=()
    command -v git   &>/dev/null || missing+=("git")
    command -v cmake &>/dev/null || missing+=("cmake")
    { command -v g++ &>/dev/null || command -v clang++ &>/dev/null; } \
        || missing+=("g++ or clang++")
    if [ ${#missing[@]} -gt 0 ]; then
        print_warning "Missing build dependencies: ${missing[*]}"
        return 1
    fi
    return 0
}

# ---------------------------------------------------------------------------
# Helper: download the latest FAE_Linux release binary from GitHub.
# Usage: download_latest_release <destination_path>
# ---------------------------------------------------------------------------
download_latest_release() {
    local dest="$1"
    # GitHub's stable "latest release" redirect
    local download_url="https://github.com/UnlegitSenpaii/FAE_Linux/releases/latest/download/FAE_Linux"

    # Prefer curl, fall back to wget
    local dl_cmd=""
    if   command -v curl &>/dev/null; then dl_cmd="curl"
    elif command -v wget &>/dev/null; then dl_cmd="wget"
    else
        print_error "Neither curl nor wget is available. Cannot download FAE_Linux."
        return 1
    fi

    # Download to a temp file first so a failed/interrupted transfer never
    # leaves a partial or corrupt binary at the final destination path.
    local dest_tmp
    dest_tmp="$(mktemp "${dest}.XXXXXX")" || {
        print_error "Could not create temporary file for download."
        return 1
    }

    print_info "Downloading FAE_Linux from: $download_url"
    if [ "$dl_cmd" = "curl" ]; then
        # -L follows the GitHub redirect, --fail treats HTTP errors as failures.
        run_clean curl -fsSL -o "$dest_tmp" "$download_url" \
            || { rm -f "$dest_tmp"; print_error "Download failed. Check your network connection and that a release exists."; return 1; }
    else
        run_clean wget -qO "$dest_tmp" "$download_url" \
            || { rm -f "$dest_tmp"; print_error "Download failed. Check your network connection and that a release exists."; return 1; }
    fi

    # Verify the downloaded file is non-empty.
    if [ ! -s "$dest_tmp" ]; then
        rm -f "$dest_tmp"
        print_error "Downloaded file is empty — the release may not have a binary asset yet."
        return 1
    fi

    # Verify the downloaded binary matches this system's architecture before
    # replacing any existing binary.
    if command -v file &>/dev/null; then
        local arch file_out arch_ok
        arch="$(uname -m)"
        file_out="$(file "$dest_tmp" 2>/dev/null)"
        arch_ok=true
        case "$arch" in
            x86_64)  printf '%s' "$file_out" | grep -qE 'x86-64|x86_64'      || arch_ok=false ;;
            aarch64) printf '%s' "$file_out" | grep -qE 'aarch64|ARM aarch64' || arch_ok=false ;;
        esac
        if [ "$arch_ok" = false ]; then
            rm -f "$dest_tmp"
            print_error "Downloaded binary architecture does not match this system ($arch)."
            print_error "file output: $file_out"
            return 1
        fi
    fi

    mv "$dest_tmp" "$dest"
    chmod +x "$dest"
    print_success "FAE_Linux downloaded to: $dest"
}

# ---------------------------------------------------------------------------
# 1. Resolved paths (derived from configuration above)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FACTORIO_BIN_DIR="$SCRIPT_DIR/$FACTORIO_BIN_SUBDIR"
FACTORIO_BIN="$FACTORIO_BIN_DIR/$FACTORIO_BIN_NAME"
FACTORIO_PATCHED_BIN="$FACTORIO_BIN_DIR/$FACTORIO_PATCHED_NAME"
FAE_LINUX_BIN="$SCRIPT_DIR/$FAE_LINUX_BIN_NAME"
FAE_LINUX_REPO_TMP="$SCRIPT_DIR/$FAE_LINUX_BUILD_DIR_NAME"
FAE_LOG_FILE="$SCRIPT_DIR/fae_launch.log"
FAE_STAGE="startup"

if [ ! -t 1 ]; then
    {
        printf '\n[FAE] ===== %s =====\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
        printf '[FAE] Headless launch detected\n'
        printf '[FAE] Script: %s\n' "$0"
        printf '[FAE] Args:'
        printf ' %q' "$@"
        printf '\n'
        printf '[FAE] PWD: %s\n' "$(pwd)"
    } >> "$FAE_LOG_FILE" 2>/dev/null || true

    exec >> "$FAE_LOG_FILE" 2>&1
    trap 'rc=$?; printf "[FAE] Exit stage=%s code=%d\n" "$FAE_STAGE" "$rc"' EXIT
fi

# ---------------------------------------------------------------------------
# Sanity-check that the original factorio binary exists
# ---------------------------------------------------------------------------
if [ ! -f "$FACTORIO_BIN" ]; then
    print_error "Factorio binary not found at: $FACTORIO_BIN"
    print_error "Make sure this script is placed in the Factorio root directory."
    exit 1
fi

# ---------------------------------------------------------------------------
# Helper: get BuildID[sha1] from a binary using 'file'
# ---------------------------------------------------------------------------
get_build_id() {
    local binary="$1"
    # Avoid grep -P (PCRE) — not available in BusyBox/Alpine environments.
    # Prefer 'file' to read ELF metadata; fall back to 'readelf' (binutils) if absent.
    if command -v file &>/dev/null; then
        file "$binary" 2>/dev/null \
            | sed -n 's/.*BuildID\[sha1\]=\([0-9a-f]*\).*/\1/p'
    elif command -v readelf &>/dev/null; then
        readelf -n "$binary" 2>/dev/null \
            | sed -n 's/.*Build ID: \([0-9a-f]*\).*/\1/p' | head -1
    fi
    # Returns empty string if neither tool is available; callers handle this.
}

ORIGINAL_BUILD_ID="$(get_build_id "$FACTORIO_BIN")"
FAE_STAGE="build-id-check"
if [ -z "$ORIGINAL_BUILD_ID" ]; then
    print_warning "Could not determine Factorio BuildID ('file'/'readelf' unavailable)."
    print_warning "Up-to-date check disabled — will always re-patch when needed."
fi
print_info "Factorio BuildID[sha1]: ${ORIGINAL_BUILD_ID:-(unknown)}"

# ---------------------------------------------------------------------------
# 2. Check if the patched binary is up-to-date
# ---------------------------------------------------------------------------
needs_patch=true

if [ -f "$FACTORIO_PATCHED_BIN" ] && [ -f "$FAE_LINUX_BIN" ] && [ -n "$ORIGINAL_BUILD_ID" ]; then
    PATCHED_BUILD_ID="$(get_build_id "$FACTORIO_PATCHED_BIN")"
    if [ -n "$PATCHED_BUILD_ID" ] && [ "$PATCHED_BUILD_ID" = "$ORIGINAL_BUILD_ID" ]; then
        print_success "Patched binary is up-to-date (BuildID matches). Launching..."
        needs_patch=false
    else
        print_info "BuildID mismatch — Factorio was updated, re-patching is needed."
    fi
else
    print_info "Patched binary or FAE_Linux patcher not found."
fi

# ---------------------------------------------------------------------------
# Build FAE_Linux (if needed), copy factorio → factorio_patched, patch it
# ---------------------------------------------------------------------------
if [ "$needs_patch" = true ]; then
    FAE_STAGE="prepare-patch"

    needs_rebuild=false

    case "${FAE_UPDATE_MODE,,}" in
        keep)
            if [ ! -f "$FAE_LINUX_BIN" ]; then
                print_error "FAE_UPDATE_MODE=keep but no FAE_Linux binary found at: $FAE_LINUX_BIN"
                print_error "Place a pre-compiled binary there or change FAE_UPDATE_MODE to \"prompt\" or \"auto-update\"."
                exit 1
            fi
            print_info "FAE_UPDATE_MODE=keep — using existing FAE_Linux binary."
            ;;
        auto-update)
            print_info "FAE_UPDATE_MODE=auto-update — pulling latest from GitHub."
            needs_rebuild=true
            ;;
        *) # prompt (default)
            if [ -f "$FAE_LINUX_BIN" ]; then
                echo
                print_warning "An existing FAE_Linux patcher was found at: $FAE_LINUX_BIN"
                print_warning "Before updating from GitHub you should verify the repository:"
                print_warning "  $FAE_LINUX_REPO_URL"
                echo
                if [ -t 0 ]; then
                    read -rp "  [e] Use existing patcher   [u] Update from GitHub   (e/u, default: e): " UPDATE_CHOICE
                else
                    UPDATE_CHOICE="e"
                    printf '[FAE] %s Headless mode: FAE_UPDATE_MODE=prompt defaults to keep. Set FAE_UPDATE_MODE=auto-update to enable automatic updates in headless mode.\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")" >> "$FAE_LOG_FILE" 2>/dev/null || true
                fi
                case "${UPDATE_CHOICE,,}" in
                    e|existing) print_info "Using existing FAE_Linux binary." ;;
                    *)          needs_rebuild=true ;;
                esac
            else
                echo
                print_warning "FAE_Linux patcher not found — it needs to be built from source."
                print_warning "This will clone and compile code from:"
                print_warning "  $FAE_LINUX_REPO_URL  (branch: $FAE_LINUX_BRANCH)"
                print_warning "Always verify the repository before proceeding!"
                echo
                if [ -t 0 ]; then
                    read -rp "  Proceed with build from GitHub? [Y/n]: " BUILD_CHOICE
                    case "${BUILD_CHOICE,,}" in
                        n|no)
                            print_error "Build cancelled."
                            print_error "To use an existing binary place it at: $FAE_LINUX_BIN"
                            exit 1
                            ;;
                        *) needs_rebuild=true ;;
                    esac
                else
                    # Headless + prompt mode + no binary — cannot ask the user.
                    # Download the latest pre-built release binary as a safe fallback
                    # (e.g. Steam Deck gaming mode where no terminal is available).
                    printf '[FAE] %s Headless+prompt: no FAE_Linux binary found; downloading pre-built release binary.\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")" >> "$FAE_LOG_FILE" 2>/dev/null || true
                    print_info "Headless mode: no binary found — downloading pre-built release binary..."
                    download_latest_release "$FAE_LINUX_BIN" || {
                        print_error "Could not obtain FAE_Linux binary automatically."
                        print_error "Set FAE_UPDATE_MODE=\"auto-update\" or place a pre-compiled binary at:"
                        print_error "  $FAE_LINUX_BIN"
                        exit 1
                    }
                fi
            fi
            ;;
    esac

    # -----------------------------------------------------------------------
    # 3. Obtain FAE_Linux: compile from source when all build dependencies are
    #    present, otherwise fall back to the latest pre-built release binary.
    # -----------------------------------------------------------------------
    if [ "$needs_rebuild" = true ]; then
        FAE_STAGE="rebuild-patcher"
        if check_build_deps; then
            print_info "Cloning FAE_Linux $FAE_LINUX_BRANCH branch..."

            # Clean any previous build attempt
            rm -rf "$FAE_LINUX_REPO_TMP"
            mkdir -p "$FAE_LINUX_REPO_TMP"

            run_clean git clone --depth 1 -b "$FAE_LINUX_BRANCH" \
                "$FAE_LINUX_REPO_URL" \
                "$FAE_LINUX_REPO_TMP" || {
                print_error "Failed to clone FAE_Linux repository."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            print_info "Building FAE_Linux..."
            BUILD_DIR="$FAE_LINUX_REPO_TMP/cmake_build"
            mkdir -p "$BUILD_DIR"

            run_clean cmake -S "$FAE_LINUX_REPO_TMP" \
                  -B "$BUILD_DIR" \
                  -DCMAKE_BUILD_TYPE=Release || {
                print_error "cmake configuration failed."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            run_clean cmake --build "$BUILD_DIR" --config Release -- -j"$(nproc 2>/dev/null || echo 1)" || {
                print_error "Build failed."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            }

            # Locate the compiled binary (CMakeLists sets output to <binary_dir>/out/bin/)
            BUILT_BIN="$(find "$BUILD_DIR/out/bin" -name "FAE_Linux" -type f 2>/dev/null | head -1)"
            if [ -z "$BUILT_BIN" ]; then
                print_error "Could not locate built FAE_Linux binary."
                rm -rf "$FAE_LINUX_REPO_TMP"
                exit 1
            fi

            # Copy only the final binary to the Factorio root; discard all build artefacts
            cp "$BUILT_BIN" "$FAE_LINUX_BIN"
            chmod +x "$FAE_LINUX_BIN"
            rm -rf "$FAE_LINUX_REPO_TMP"
            print_success "FAE_Linux built and installed to: $FAE_LINUX_BIN"
        else
            print_warning "One or more build dependencies are missing (see above)."
            print_warning "Falling back to downloading the latest pre-built release binary."
            download_latest_release "$FAE_LINUX_BIN" || {
                print_error "Could not obtain FAE_Linux binary. Aborting."
                exit 1
            }
        fi
    fi

    # -----------------------------------------------------------------------
    # 4. Copy factorio → factorio_patched and run the patcher
    # -----------------------------------------------------------------------
    FAE_STAGE="patch-factorio"
    print_info "Copying factorio → factorio_patched..."
    cp "$FACTORIO_BIN" "$FACTORIO_PATCHED_BIN"

    print_info "Running FAE_Linux patcher on factorio_patched..."
    # Only suppress the interactive prompt when there is no terminal to show it in.
    FAE_EXTRA_ARGS=()
    if [ ! -t 1 ]; then
        FAE_EXTRA_ARGS+=("--no-prompt")
    fi
    "$FAE_LINUX_BIN" "$FACTORIO_PATCHED_BIN" "${FAE_EXTRA_ARGS[@]+"${FAE_EXTRA_ARGS[@]}"}" || {
        PATCHER_EXIT=$?
        print_error "FAE_Linux patcher exited with code $PATCHER_EXIT"
        rm -f "$FACTORIO_PATCHED_BIN"
        exit $PATCHER_EXIT
    }

    print_success "Patching completed successfully!"
fi

# ---------------------------------------------------------------------------
# 5. Launch factorio_patched
# ---------------------------------------------------------------------------
FAE_STAGE="launch-factorio"
print_info "Launching factorio_patched..."

# Restore Steam's library environment so Factorio can find Steam's own libs
export LD_LIBRARY_PATH="$STEAM_LD_LIBRARY_PATH"
export LD_PRELOAD="$STEAM_LD_PRELOAD"

# If Steam launched us via %command%, replay the original runtime/wrapper chain
# but replace the final Factorio binary path with the patched one. This keeps
# Steam's own launch semantics intact and avoids passing wrapper-only flags such
# as --oom-score-adjust directly to Factorio.
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

# Fallback for manual invocation outside Steam where the original Factorio
# binary path is not present in the arguments.
if [ "$USED_STEAM_CHAIN" = false ]; then
    LAUNCH_CMD=("$FACTORIO_PATCHED_BIN" "$@")
fi

# Replace this wrapper process with the actual launch target so Steam keeps
# tracking the launched app correctly in Gaming Mode.
print_success "Factorio launched. Handing control to Steam..."
exec "${LAUNCH_CMD[@]}"



#                                ,╓æ╥╦φφφ╥╥¿
#                         ,▄ñ▒▒▀▀▒▒▒▒▒▒▒▒▒▒▒▒▒N▄,«mw,
#                     a∞▓▀░░░Z▒▒▒µ▐╨▒▒▒▒▒▒▒▒▒▒╨░░░▒▒░┘*
#                 ╓æ╨,▒╩░░░░▒░░░T▒▒W░░▒╩ÑÑ▒╩▐░≥▒╩╨▒T▒   ╙v
#               ¿╨,╦░ñ░░░░░î░░░░░▒▒░░░░░░░░░░░▒░░░░░░░x   ²╕  ╓▄▓▓▄
#      ╓╓,,   ▄└,░░░░▄▄▄▄░░▒░.▒░░╫▒░░░░░░▒░░░░▒▒░░░░░░░▒    ╨█▓▓▓▓▓
#      ▓▓▓▓▓▓▓▄▓▓▄▓▓▓▓▓▓▓▌Æ░░▒▒░▒▒░░░░░░░]1░░░░▌▒░▒░░░V░▒   `Γ█▓▓▓▓
#      █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█ƒ▒░░░░░▒╫▒░░▒░░░░▒░░░░▒▒▒░░░░░▒░░    V█▓▓▓▓,
#      ╫▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▌╫░▒░░░░▒▒φ░░▒░░░░░░░░░▒▒H░▒░░░╫▒░▒    ╙▓▓▓▓▓
#      ║▓▓▓▓▓▓▓▓██▓▓▓▓▓▓▌]░▒░░░▒▒M░░░▒▒░░░░░░░░▒▒░░▒▒░▒h╫▒░▒    `█▓▓▓▌
#      ╫▓▓▓▓█▓▓▌█████▓▓█▒j░▒░░»░▒▒\"Å░▒▒▒░░▒░░░▒▒M∞▒▒▒▒▒░░R▒░\\   ╫▄██▀ 
#       ²╙▐▓▓▓▓▒█▓▓▓▒▐░▐░░░Ü░░░░▒░  *╫▒▒▒▒▒░░░╫Ñ░ │▒▒▒▒░░░▀▒░µ  ▐▄\"▀,
#         ▓▓▓▓▌▒▓▓▓▓▓░▒▐⌐▐\"Ü░░░░╨┴░  `Ü*▒▒▒▒░░▓I▄▄▓▓▒░╡░░░░▐░▒  ]░▌
#        █▌░▀▓▒▒╫▀▀▀▀M▒└ ▄▓▓▓▓█▓▓▀▀h   ª÷╨▒▒▒░▐╨█████▓▓▀██M ▌░  ▌▒░▄
#         ─░░▐▒▒░░░░▒² ,MÑ▄█▀╩███▓╨\"        ª╨░▓▓ `▓██▓╩╩░░▒▌µL▐▒░░▐µ
#        Å]░▒▒▒▒░░░²  4▐░░▀╫Mm▀Ñ▀Ü░        ░░  ╨▒MMÑ▒▀Ü░░░░░╫▒E░░░¼░▓
#       J ▒░Ñ\"\"╞  ▒µ,▒▒░░░░≡∞ª`k░                ░░░░░░░░░░░░▒░░▒▒░▌░ 
#      J ╦▒▒▒▒╦╫▒▒╨î░▒\"`     »C            ░:░    ░░░░░░░░░░░░▒░▒▒░▌▒▀
#     /,▒▓▒Ñ░▒▒▒▒░  ▒░µ.¿⌐░░░░      *,              ░░░░░░░░░░╫░▒▒▒▌`▐µ
#   ╓Pª` ▌▒░░░▒╨╨ª\"  ╨▒▒|░░░           \"=─~---        ░░░░░░░,Ü░▒▒▒▌ ╞h
#        ▌▒░░╛      ,ó▒▒▒                                ░░ æ▒░░▒▒╫  ▐─
#        ▐Ü    .,,φ▒▒▒░▒▒b                              ░y╦▒▒▌░░╫▒   ▀
#        ▌      ▓▒▌µÅ▒▒░▒▒▓Ww,                       ,╥ß▒▒▒▒▒▒░░▌
#       JH      ▓▒▌└µ▌╫▒░░░▒▒▒▒▒N╥µ,          ,,¿╥ß▒▒▒▒▒▒░░▒▒Ü▐▀▌
#       Æ      JH \" ▐╨Ö╙╨╨▒≥░▒▒▒▒ÑÑ▒▓░░▒▒▒▒▒▒░▓▓▒▒▒▒▒▒▒▒▒░░▐█Å└ '
#      JH      JUÆ▒▒▒,       ▀▒▒▒▒▒╫▓╦░░░░▒▒╥▄▒▓Ñ╫Ñ▒▒▒░░░░░▓┘
#      Æ        ▒░╫▒1▒N    ░  ²▒▒Ñ▒Ñ▒▒▒▒▒▒Ñ▒╫Ñ╫▒▒ÜÑ▒▒▌\"╨µ░ƒ▒▀▄
#     J.        ░░░╫µ▒b▒        ╩N▒╩KÑ▒▒╨▒▒▒▒╫▒ÑA▒▒▓▒  ╒ ¥▒▒▒░▒
#     ▐        ▐░░░▓M▒▒░▒        `%▒▒▒▒▒╦╥▒H╩╫▌▒▓▓▓▒╛ ▒F  \\▒░░▐
#      U       ▒░░░▓Ñ▓╫░▒          ╙╫ÑÑ╫▓╫▓▓▓▌▒▓▓▓╩╨   L   R░░▒Nµ
#      `w;¡¿µw╩╨╨▀▓▌Ñ╫╫`              ``  ,@▓▓╙▀▓╫▓@,  `µ  ░░%▒▒▒▓
#                 ╙²ª╨,   ,╓╥╦▒ª         ╒▒╫▓▌  ▓▓▒ÑÑφ  ▐  ░░≥▒▒▒▌