#!/bin/bash

#    ███████╗░█████╗░███████╗  ██╗░░░░░██╗███╗░░██╗██╗░░░██╗██╗░░██╗
#    ██╔════╝██╔══██╗██╔════╝  ██║░░░░░██║████╗░██║██║░░░██║╚██╗██╔╝
#    █████╗░░███████║█████╗░░  ██║░░░░░██║██╔██╗██║██║░░░██║░╚███╔╝░
#    ██╔══╝░░██╔══██║██╔══╝░░  ██║░░░░░██║██║╚████║██║░░░██║░██╔██╗░
#    ██║░░░░░██║░░██║███████╗  ███████╗██║██║░╚███║╚██████╔╝██╔╝╚██╗
#    ╚═╝░░░░░╚═╝░░╚═╝╚══════╝  ╚══════╝╚═╝╚═╝░░╚══╝░╚═════╝░╚═╝░░╚═╝

# Automation script for Factorio Achievement Enabler for Linux -- written with AI assistance

# 1. Place this script in your Factorio directory. Example:
#   /home/<user>/.local/share/Steam/steamapps/common/Factorio/

# 2. Set as Steam launch option (with full path!!). Example:
#   bash /home/<user>/.local/share/Steam/steamapps/common/Factorio/fae_launch.sh %command%

# ---------------------------------------------------------------------------
# Configuration — edit these to match your setup
# ---------------------------------------------------------------------------

# Controls how FAE_Linux updates are handled when a new patch is needed.
#   prompt      — ask each time (default, recommended)
#   keep        — always use the existing binary without asking
#   auto-update — always pull and rebuild from GitHub without asking
FAE_UPDATE_MODE="prompt"

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
if [ ! -t 1 ] && [ -z "${FAE_IN_TERMINAL:-}" ]; then
    SELF="$(readlink -f "${BASH_SOURCE[0]}")"

    # Write a temp launcher script to avoid quoting issues with "$@"
    LAUNCHER_TMP="$(mktemp /tmp/fae_launch_XXXXXX.sh)"
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

    # Try common terminal emulators in order of preference.
    # gnome-terminal needs --wait so Steam does not think the game already quit.
    # All others run in the foreground by default when exec'd.
    for term in xterm gnome-terminal konsole xfce4-terminal alacritty kitty; do
        if command -v "$term" &>/dev/null; then
            case "$term" in
                gnome-terminal) exec gnome-terminal --wait -- bash "$LAUNCHER_TMP" ;;
                konsole)        exec konsole -e bash "$LAUNCHER_TMP" ;;
                xfce4-terminal) exec xfce4-terminal -e "bash $LAUNCHER_TMP" ;;
                alacritty)      exec alacritty -e bash "$LAUNCHER_TMP" ;;
                kitty)          exec kitty bash "$LAUNCHER_TMP" ;;
                xterm)          exec xterm -e bash "$LAUNCHER_TMP" ;;
                # Is your favorite terminal missing here? Feel free to create a PR with it added :)
                # Know kow to do this better? please i beg you, please 
            esac
        fi
    done

    # No terminal emulator found — remove the temp file and continue headlessly.
    rm -f "$LAUNCHER_TMP"
    printf '[WARNING] No terminal emulator found. Running without a visible window.\n' >&2
fi

set -euo pipefail

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
# 1. Resolved paths (derived from configuration above)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FACTORIO_BIN_DIR="$SCRIPT_DIR/$FACTORIO_BIN_SUBDIR"
FACTORIO_BIN="$FACTORIO_BIN_DIR/$FACTORIO_BIN_NAME"
FACTORIO_PATCHED_BIN="$FACTORIO_BIN_DIR/$FACTORIO_PATCHED_NAME"
FAE_LINUX_BIN="$SCRIPT_DIR/$FAE_LINUX_BIN_NAME"
FAE_LINUX_REPO_TMP="$SCRIPT_DIR/$FAE_LINUX_BUILD_DIR_NAME"

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
    file "$binary" 2>/dev/null | grep -oP 'BuildID\[sha1\]=\K[0-9a-f]+'
}

ORIGINAL_BUILD_ID="$(get_build_id "$FACTORIO_BIN")"
if [ -z "$ORIGINAL_BUILD_ID" ]; then
    print_error "Could not determine BuildID for $FACTORIO_BIN"
    exit 1
fi
print_info "Original BuildID[sha1]: $ORIGINAL_BUILD_ID"

# ---------------------------------------------------------------------------
# 2. Check if the patched binary is up-to-date
# ---------------------------------------------------------------------------
needs_patch=true
fae_bin_exists=false
[ -f "$FAE_LINUX_BIN" ] && fae_bin_exists=true

if [ -f "$FACTORIO_PATCHED_BIN" ] && [ "$fae_bin_exists" = true ]; then
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

    needs_rebuild=false

    if [ "$fae_bin_exists" = true ]; then
        # FAE_Linux binary already present — decide whether to update based on mode.
        case "${FAE_UPDATE_MODE,,}" in
            auto-update)
                print_info "FAE_UPDATE_MODE=auto-update — pulling latest from GitHub."
                needs_rebuild=true
                ;;
            keep)
                print_info "FAE_UPDATE_MODE=keep — using existing FAE_Linux binary."
                ;;
            *) # prompt (default)
                echo
                print_warning "An existing FAE_Linux patcher was found at: $FAE_LINUX_BIN"
                print_warning "Before updating from GitHub you should verify the repository:"
                print_warning "  $FAE_LINUX_REPO_URL"
                echo
                if [ -t 0 ]; then
                    read -rp "  [e] Use existing patcher   [u] Update from GitHub   (e/u, default: u): " UPDATE_CHOICE
                else
                    UPDATE_CHOICE="u"
                fi
                case "${UPDATE_CHOICE,,}" in
                    u|update) needs_rebuild=true ;;
                    *)        print_info "Using existing FAE_Linux binary." ;;
                esac
                ;;
        esac
    else
        # No binary at all — must build from source.
        case "${FAE_UPDATE_MODE,,}" in
            keep)
                print_error "FAE_UPDATE_MODE=keep but no FAE_Linux binary found at: $FAE_LINUX_BIN"
                print_error "Place a pre-compiled binary there or change FAE_UPDATE_MODE to \"prompt\" or \"auto-update\"."
                exit 1
                ;;
            auto-update)
                print_info "FAE_UPDATE_MODE=auto-update — building FAE_Linux from source."
                needs_rebuild=true
                ;;
            *) # prompt (default)
                echo
                print_warning "FAE_Linux patcher not found — it needs to be built from source."
                print_warning "This will clone and compile code from:"
                print_warning "  $FAE_LINUX_REPO_URL  (branch: $FAE_LINUX_BRANCH)"
                print_warning "Always verify the repository before proceeding!"
                echo
                if [ -t 0 ]; then
                    read -rp "  Proceed with build from GitHub? [y/N]: " BUILD_CHOICE
                    case "${BUILD_CHOICE,,}" in
                        y|yes) needs_rebuild=true ;;
                        *)
                            print_error "Build cancelled."
                            print_error "To use an existing binary place it at: $FAE_LINUX_BIN"
                            exit 1
                            ;;
                    esac
                else
                    # Headless — build automatically
                    print_info "No terminal detected — building FAE_Linux automatically."
                    needs_rebuild=true
                fi
                ;;
        esac
    fi

    # -----------------------------------------------------------------------
    # 3. Clone and compile FAE_Linux (only when the user asked for an update
    #    or no binary was present at all)
    # -----------------------------------------------------------------------
    if [ "$needs_rebuild" = true ]; then
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

        run_clean cmake --build "$BUILD_DIR" --config Release -- -j"$(nproc)" || {
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
    fi

    # -----------------------------------------------------------------------
    # 4. Copy factorio → factorio_patched and run the patcher
    # -----------------------------------------------------------------------
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
print_info "Launching factorio_patched..."

# Restore Steam's library environment so Factorio can find Steam's own libs
export LD_LIBRARY_PATH="$STEAM_LD_LIBRARY_PATH"
export LD_PRELOAD="$STEAM_LD_PRELOAD"

# Forward any extra arguments (e.g., Steam's %command% tail) to the binary.
# Steam typically passes the original binary path as the last positional
# argument; we drop it and keep everything else.
EXTRA_ARGS=()
for arg in "$@"; do
    # Skip the original factorio binary path so we don't double-launch it
    if [ "$arg" != "$FACTORIO_BIN" ]; then
        EXTRA_ARGS+=("$arg")
    fi
done

# Launch detached from this terminal so the window closes immediately on success.
# setsid creates a new process session so the child is immune to the terminal's
# SIGHUP when it closes. disown removes it from the shell's job table.
# All output is redirected to /dev/null — Factorio has its own log files.
setsid nohup "$FACTORIO_PATCHED_BIN" "${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}" &>/dev/null &
disown $!

print_success "Factorio launched. Closing window..."



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