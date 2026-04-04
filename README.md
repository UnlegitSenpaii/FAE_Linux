# FAE_Linux - Factorio Achievement Enabler for Linux

**FAE_Linux** (Factorio Achievement Enabler for Linux) allows you to unlock Steam achievements while playing the game with mods on Linux.

## Features
- **Unlock Steam Achievements:** Enables achievement tracking with mods.
- **Easy to Use:** Simple command-line interface.
- **Automated Steam Integration:** The included `fae_launch.sh` script handles patching and launching automatically as a Steam launch option.

## Usage

### Automatic Setup (Recommended)
#### Steam Launch Script:
The `fae_launch.sh` script automates the entire workflow — it detects Factorio updates, (re-)patches the binary when needed, and launches the patched game. Set it once as a Steam launch option and forget about it.

**Setup:**
1. **Copy the script** to your Factorio root directory (the folder containing `bin/`, `data/`, etc.).
   ```sh
   # Example path
   ~/.local/share/Steam/steamapps/common/Factorio/
   ```
2. **Set it as a Steam launch option.** In Steam, right-click Factorio → Properties → Launch Options and enter the full path:
   ```sh
   bash /home/<user>/.local/share/Steam/steamapps/common/Factorio/fae_launch.sh %command%
   ```

**What it does each launch:**
- Compares the BuildID of the original `factorio` binary against the already-patched copy.
- If they match, it skips patching and launches immediately.
- If Factorio was updated (BuildID mismatch), it re-patches before launching.
- If no FAE_Linux patcher binary is present, it can clone and build one from source automatically.

**Configuration:**

Open `fae_launch.sh` and edit the variables at the top to suit your setup. The most important one is `FAE_UPDATE_MODE`:

| Value | Behaviour |
|---|---|
| `prompt` | Ask each time whether to use the existing patcher or pull a fresh build from GitHub. **(default)** |
| `keep` | Always use the existing FAE_Linux binary without asking. |
| `auto-update` | Always clone and rebuild FAE_Linux from GitHub without asking. |

*Note: On Steam Deck, replace `<user>` with `deck` in the paths above.*

---

### Manual Setup
#### Universal:
1. **Download:** Get the executable file from the [releases tab](https://github.com/UnlegitSenpaii/FAE_Linux/releases).
2. **Mark the file as executable:** Apply the executable flag to the downloaded file.
   ```sh
   chmod +x ./Downloads/FAE_Linux
   ```
3. **Run:** Execute it with the Factorio path as a parameter.
   ```sh
   ./Downloads/FAE_Linux /path/to/Factorio/bin/x64/factorio
   ```
*Note: If running the executable does not work, you will have to build the project from source.*

#### Steam Deck:
1. **Download:** Get the executable file from the [releases tab](https://github.com/UnlegitSenpaii/FAE_Linux/releases).
2. **Open the Terminal:** Search and Open the "Konsole" Application.
3. **Mark the file as executable:** Apply the executable flag to the downloaded file.
   ```sh
   chmod +x ./Downloads/FAE_Linux
   ```
4. **Run:** Execute it with the Factorio path as a parameter.
   ```sh
   ./Downloads/FAE_Linux  /home/deck/.local/share/Steam/steamapps/common/Factorio/bin/x64/factorio
   ```
*Note: This assumes you installed the game on your local drive and not on an SD Card*

#### NixOS (not officially supported):
If you're on NixOS or have Nix installed with Flakes enabled, you can instead use
```sh
nix run github:UnlegitSenpaii/FAE_Linux /path/to/Factorio/bin/x64/factorio
```

## Building from Source
Follow these steps to build the binary yourself:
1. Clone the repository:
    ```sh
    git clone https://github.com/UnlegitSenpaii/FAE_Linux.git
    ```
2. Compile the project:
    ```sh
    cmake FAE_Linux/CMakeLists.txt
    make
    ```
3. The compiled binary can be found in the /out folder.

## Contribution Guidelines

- **Major cosmetic changes** will not be accepted and will be **rejected**.
- Small, meaningful changes such as **pattern updates** or minor functionality improvements are **welcome**.
- Pull requests that introduce **large, sweeping changes** that are difficult to review will be rejected **without comment**, due to **security and maintainability concerns**.
Ensure your contributions are focused and easy to review.

## Additional Information
- [**Wiki**](https://github.com/UnlegitSenpaii/FAE_Linux/wiki): Find detailed guides and documentation.
- **Release Notes**: Check the latest release note information about the supported versions and other test results.

[![CodeQL](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/codeql.yml/badge.svg)](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/codeql.yml)
[![C/C++ CI](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/c-cpp.yml)


## Credits
[oorzkws FAE for Windows (for the Patterns :) )](https://github.com/oorzkws/FactorioAchievementEnabler)<br>
[contributors](https://github.com/UnlegitSenpaii/FAE_Linux/graphs/contributors)<br>

## Tools Used
[VSCode](https://code.visualstudio.com/)<br>
[Ghidra](https://github.com/NationalSecurityAgency/ghidra)<br>
