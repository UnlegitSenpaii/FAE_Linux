# FAE_Linux
Factorio Achievement Enabler for Linux

With this you can get steam achievements while playing the game with mods.

## Usage
Get the executable file from the release tab and run it with the factorio path as a parameter. <br>
Example: <br>
`./FAE_Linux /home/senpaii/steamdrives/nvme1/SteamLibrary/steamapps/common/Factorio/bin/x64/factorio`

## Building
You can build the binary yourself using `cmake CMakeLists.txt` followed with `make`.

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

## How to get the Windows game files from steam, without affecting the current install

### Before running the script
make sure that you have steamcmd installed. <br>
[how?](https://developer.valvesoftware.com/wiki/SteamCMD#Downloading_SteamCMD)

### Creating a script
Create a text file with the content below.<br>
Example: `nano update_factorio_x64_windows.txt`<br>
```
@ShutdownOnFailedCommand 1
@NoPromptForPassword 1
@sSteamCmdForcePlatformType windows
force_install_dir ../../factorio_windows_x64
login STEAM_USERNAME
app_update 427520 validate
quit
```
Replace `STEAM_USERNAME` with your logon name in steam (you might have to have steam open).<br>
**Important!** `force_install_dir` in the script, assumes your steam folder is in ~/.steam !

[more](https://developer.valvesoftware.com/wiki/SteamCMD#Creating_a_Script)

### Running the script
Finally, run the script with `steamcmd +runscript update_factorio_x64_windows.txt` <br>
After downloading, the game files can be found in ~/factorio_windows_x64

[steam cmd documentation](https://developer.valvesoftware.com/wiki/SteamCMD)<br>

*fuck the factorio discord (modding) community*
