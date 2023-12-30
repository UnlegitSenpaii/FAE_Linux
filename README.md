# FAE_Linux
Factorio Achievement Enabler for Linux

With this you can get steam achievements while playing the game with mods.

## Usage
Get the executable file from the release tab and run it with the factorio path as a parameter. <br>
Example: <br>
`./FAE_Linux /home/senpaii/steamdrives/nvme1/SteamLibrary/steamapps/common/Factorio/bin/x64/factorio`

## Building
You can build the binary yourself using `cmake CMakeLists.txt`.

## Notes
Tested on OS: Arch Linux x86_64 Kernel: 6.0.12-arch1-1 

Tested by @Jano1312 on Factorio Version 1.1.80

Get the patterns from ghidra.<br>
I used the windows patterns as a reference, this makes it very easy to find the patterns.<br>
If there is an update you can just compare the old and new file versions, no need to use the windows version. <br>
Error during DWARFAnalyzer import: Only DWARF version 2, 3, or 4 information is currently supported (detected 5). (as of 06.03.2023)  :( <br>

The tool doesn't currently remove the "The game is modded." message,  
but that's on the roadmap for future updates.  
The necessary patterns for this are AchievementGUI::AchievementGUI and PlayerData::PlayerData.  
Despite the warning in-game, the patch is functional, and you should still be able to unlock achievements.

## Credits
[CLion (Best IDE)](https://www.jetbrains.com/clion/)<br>
[Ghidra (Tool used for RE)](https://github.com/NationalSecurityAgency/ghidra)<br>
[oorzkws FAE for Windows (for the Patterns :) )](https://github.com/oorzkws/FactorioAchievementEnabler)<br>

## how to get the Windows game files from steam and keeping the current install

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
