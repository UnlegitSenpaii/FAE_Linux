# FAE_Linux - Factorio Achievement Enabler for Linux

**FAE_Linux** (Factorio Achievement Enabler for Linux) allows you to unlock Steam achievements while playing the game with mods on Linux.

## Features
- **Unlock Steam Achievements:** Enables achievement tracking with mods.
- **Easy to Use:** Simple command-line interface.

## Usage
1. **Download:** Get the executable file from the [releases tab](https://github.com/UnlegitSenpaii/FAE_Linux/releases).
2. **Mark the file as executable:** Apply the executable flag to the downloaded file.
   ```sh
   chmod +x Downloads/FAE_Linux
   ```
3. **Run:** Execute it with the Factorio path as a parameter.
   ```sh
   ./FAE_Linux /path/to/Factorio/bin/x64/factorio
   ```
*Note: If running the executable does not work, you will have to build the project from source.* 

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

## Credits
[oorzkws FAE for Windows (for the Patterns :) )](https://github.com/oorzkws/FactorioAchievementEnabler)<br>
[contributors](https://github.com/UnlegitSenpaii/FAE_Linux/graphs/contributors)<br>

## Tools Used
[VSCode](https://code.visualstudio.com/)<br>
[Ghidra](https://github.com/NationalSecurityAgency/ghidra)<br>
