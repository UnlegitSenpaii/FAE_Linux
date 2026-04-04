# FAE_Linux - Factorio Achievement Enabler for Linux

**FAE_Linux** lets you earn Steam achievements in Factorio even when mods are active — something the game normally blocks.

> **In short:** it patches the Factorio binary to remove the mod-check that disables achievements. Your save and game files are never touched.

<!-- todo: add gif or screenshot here, of patcher running?  -->


[![CodeQL](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/codeql.yml/badge.svg)](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/codeql.yml)
[![C/C++ CI](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/UnlegitSenpaii/FAE_Linux/actions/workflows/c-cpp.yml)

---

## How does it work?

Factorio disables Steam achievements whenever mods are loaded. FAE_Linux patches a single check inside the Factorio binary so that achievements keep working. The original binary is left untouched — a separate patched copy is created alongside it.

---

## Setup

Pick the method that matches how you play:

| I want to… | Use |
|---|---|
| Set it up once and never think about it again | [Automatic — Steam Launch Script](#automatic-setup-recommended) |
| Run it manually whenever I need it | [Manual](#manual-setup) |
| Build from source | [Building from Source](#building-from-source) |

---

### Automatic Setup (Recommended)

The included `fae_launch.sh` script does everything for you every time you launch Factorio through Steam:
- Checks whether Factorio was updated since the last patch.
- Re-patches automatically if needed.
- Launches the patched game.

**Step 1 — Copy the script into your Factorio folder**

Your Factorio folder is usually at:
```
~/.local/share/Steam/steamapps/common/Factorio/
```
Copy `fae_launch.sh` directly into that folder (not into `bin/` or any subfolder).

**Step 2 — Set it as a Steam launch option**

In Steam: right-click **Factorio** → **Properties** → **Launch Options**, and paste this line (replace `<user>` with your Linux username):

```sh
bash /home/<user>/.local/share/Steam/steamapps/common/Factorio/fae_launch.sh %command%
```

> **Steam Deck?** Replace `<user>` with `deck`.

That's it. Launch Factorio from Steam as normal — the script runs in the background.

**First launch behaviour**

The first time you run it (or after a Factorio update), a terminal window will appear and ask whether to use an existing FAE_Linux patcher binary or download and build one from GitHub. Choose whichever you prefer and the window will close automatically once the game starts.

**Changing update behaviour**

Open `fae_launch.sh` in a text editor and find the `FAE_UPDATE_MODE` line near the top:

| Value | What happens |
|---|---|
| `prompt` | Asks you each time a new patch is needed. **(default)** |
| `keep` | Always uses the existing patcher binary without asking. |
| `auto-update` | Always pulls and rebuilds the latest patcher from GitHub without asking. |

---

### Manual Setup

Use this if you prefer to run the patcher yourself rather than having it run automatically on launch.

#### Linux (Desktop)

1. **Download** the `FAE_Linux` executable from the [Releases page](https://github.com/UnlegitSenpaii/FAE_Linux/releases).
2. **Make it executable:**
   ```sh
   chmod +x ~/Downloads/FAE_Linux
   ```
3. **Run it**, pointing it at the Factorio binary:
   ```sh
   ~/Downloads/FAE_Linux /path/to/Factorio/bin/x64/factorio
   ```

> If the executable doesn't run, your system may need you to [build from source](#building-from-source).

#### Steam Deck

1. **Download** the `FAE_Linux` executable from the [Releases page](https://github.com/UnlegitSenpaii/FAE_Linux/releases).
2. Open the **Konsole** app (search for it in Desktop Mode).
3. **Make it executable:**
   ```sh
   chmod +x ~/Downloads/FAE_Linux
   ```
4. **Run it:**
   ```sh
   ~/Downloads/FAE_Linux /home/deck/.local/share/Steam/steamapps/common/Factorio/bin/x64/factorio
   ```

> This assumes Factorio is installed on the internal drive. Adjust the path if you use an SD card.

#### NixOS *(unofficial)*

If you use Nix with Flakes enabled:
```sh
nix run github:UnlegitSenpaii/FAE_Linux /path/to/Factorio/bin/x64/factorio
```

---

## Building from Source

Only needed if the pre-built binary doesn't work on your system.

1. **Clone the repository:**
   ```sh
   git clone https://github.com/UnlegitSenpaii/FAE_Linux.git
   ```
2. **Build:**
   ```sh
   cmake FAE_Linux/CMakeLists.txt
   make
   ```
3. The compiled binary will be in the `out/` folder.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on what is welcome, what will be rejected, and how to open a pull request.

---

## Additional Information
- [**Wiki**](https://github.com/UnlegitSenpaii/FAE_Linux/wiki) — detailed guides and documentation.
- **Release Notes** — supported versions and test results are listed on each release.

---

## Credits
[oorzkws FAE for Windows (for the Patterns :) )](https://github.com/oorzkws/FactorioAchievementEnabler)<br>
[contributors](https://github.com/UnlegitSenpaii/FAE_Linux/graphs/contributors)<br>

## Tools Used
[VSCode](https://code.visualstudio.com/)<br>
[Ghidra](https://github.com/NationalSecurityAgency/ghidra)<br>
