# FAE_Linux
Factorio Achievement Enabler for Linux

With this you can get steam achievements while playing the game with mods.

## Usage
Get the executable file from the release tab and run it with the factorio path as a parameter. <br>
Example: <br>
`./FAE_Linux /.../SteamLibrary/steamapps/common/Factorio/bin/x64/factorio`

## Building
You can build the binary yourself using the following commands:
```
git clone https://github.com/UnlegitSenpaii/FAE_Linux.git
cmake FAE_Linux/CMakeLists.txt
make
```
The compiled binary can be found in the /out folder.

## Contribution Guidelines

- **Major cosmetic changes** will not be accepted and will be **rejected**.
- Small, meaningful changes such as **pattern updates**, fixing typos or minor functionality improvements are **welcome**.
- Pull requests that introduce **large, sweeping changes** that are difficult to review will be rejected **without comment**, due to **security and maintainability concerns**.

Please ensure your contributions are focused, clear, and easy to review. Thank you for your understanding!

## Notes
You can find the results of tests in the latest release notes.

Get the patterns from ghidra.<br>
I used the windows patterns as a reference, this makes it very easy to find the patterns.<br>
If there is an update you can just compare the old and new file versions, no need to use the windows version. <br>

## Credits
[VSCode](https://code.visualstudio.com/)<br>
[Ghidra (Tool used for RE)](https://github.com/NationalSecurityAgency/ghidra)<br>
[oorzkws FAE for Windows (for the Patterns :) )](https://github.com/oorzkws/FactorioAchievementEnabler)<br>
