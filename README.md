
Speeds up game start by storing INI files in memory instead of opening, parsing and closing the file each time some value from it is needed.

# Redirector
### Problem
The issue this plugin tries to solve comes from the fact that the game uses an old, deprecated and extremely inefficient function to load values from INI files. The function in question is `GetPrivateProfileString` (and the rest of `PrivateProfile` function family to lesser extent), a relic of 16-bit operating system. But do you know what is worse than using such a function? Using it hundreds of thousands of times. Apparently the game uses that function for every game setting individually, which means that the same INI file is opened and closed hundreds of times. What really makes this terrible is the fact that for each enabled plugin the game will attempt to read all the same settings from a hypothetical INI file that that plugin might or might not be loading. So you end up with an insane amount of incredibly inefficient calls for each enabled mod plugin. As a solution this mod will hook the functions involved and load the target INI files in memory so that each following call to the same INI file will be much faster as it will read directly from memory and will not require opening the file from scratch again.

The amount of seconds that this mod will reduce your start time by depends on the amount of enabled plugins you have, as well as other things using INI files such as ENB (though ENB comes with its own loading issues that this mod can't fix). It will be basically imperceptible when used on the vanilla game and will go up from there, shaving off 6 seconds for 50 or so plugins, up to 20+ seconds for 250+ plugins.

### Solution
When the game, ENB or something else calls `GetPrivateProfileString` for the first time this plugin will load requested file in memory and return required data. Next time, no file will be loaded and data will be fetched from memory as well. Same thing happens when process tries to write a value to file. Instead of opening file again, parse and save to disk the plugin will write data it to in-memory file and then saves to disk (if allowed). Also all files is saved on game close, if game won't crash in process.

##### Covered WinAPI functions
- GetPrivateProfileStringA
- GetPrivateProfileStringW
- WritePrivateProfileStringA
- WritePrivateProfileStringW
- GetPrivateProfileIntA
- GetPrivateProfileIntW
- GetPrivateProfileSectionNamesA
- GetPrivateProfileSectionNamesW
- GetPrivateProfileSectionA
- GetPrivateProfileSectionW

# Integrations
### Script Extender (XSE)
The mod supports XSE and, if used with the matching XSE build, will use XSE to bind to some of the game's events (such as game saving) and call this mod functions from the game's console.

This integration currently implements:
- `GameSave` event to save all changes to disk before the game itself is saved.
- `RefreshINI` command to discard any in-memory state and reload the files from disk. The mod doesn't add this command, it already exists in the vanilla game, it only alters it to refresh the files in addition to its normal functions.

This integration **is not** required for the mod to function and the mod doesn't require the matching XSE installation to work. Having the matching XSE build installed only allows Redirector to load and use this integration.

### ENB
Redirector also supports integration with [ENB](http://www.enbdev.com) graphical modification. This integration is used to bind to events of saving configuration in ENB's on-screen menu. This helps to avoid the loss of changes if you, say, edited ENB parameters, saved them there and closed the game, but, due to game crash on exit (happens way too often) and/or Redirector's options nothing else triggered save operation.

Due to ENB's quite limited and inconsistent API only save config event is implemented properly (handled with  `PostLoad` event). The other event, `PreReset`, assumed to be called before config load, is not triggered anywhere, but is implemented to refresh data from disk (same as `RefreshINI` from XSE integration).

# Download, requirements and configuration
- [Skyrim LE](https://www.nexusmods.com/skyrim/mods/92725)
  - [SKSE](https://www.nexusmods.com/skyrim/mods/100216)
  - [SKSE Plugin Preloader](https://www.nexusmods.com/skyrim/mods/75795)
- [Skyrim SE/AE/VR](https://www.nexusmods.com/skyrimspecialedition/mods/18860)
  - [SKSE64](https://www.nexusmods.com/skyrimspecialedition/mods/30379) for SE/AE or [SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/30457) for VR version
  - [SSE Engine Fixes](https://www.nexusmods.com/skyrimspecialedition/mods/17230) for the plugin preloader included in it. There's no Engine Fixes mod for VR, but, I believe, the "Part 2" file from SSE Engine Fixes (which is preloader) should work on VR edition.
- [Fallout 4/VR](https://www.nexusmods.com/fallout4/mods/33947)
  - [F4SE](https://www.nexusmods.com/fallout4/mods/42147) or [F4SEVR](https://www.nexusmods.com/fallout4/mods/42159) for VR version
  - [xSE PluginPreloader](https://www.nexusmods.com/fallout4/mods/33946) (this should work on VR version too)

Redirector itself doesn't need Script Extender to function. The only thing that depends on it is XSE integration. If you load Redirector without XSE or mismatched XSE build (older or newer version of it) the Redirector will not load XSE integration, but still will function as usual. So yes, **you can use this mod with any version of the game and/or XSE**.

Plugin can be configured in its own INI file, each parameter is described inside this file.

# Q&A
**Q:** Do I really need this?  
**A:** It depends. If your game starts from shortcut to main menu in split second then no. If it takes longer, this plugin can help. How much time you will save depends on your setup.

**Q:** Will this work with MO/NMM/Vortex/[Kortex](https://nexusmods.com/skyrim/mods/90868) (yeah, I had to mention it) or any other mod manager?  
**A:** It was tested with MO2 and Kortex and no problems was found. Vortex and NMM have not been tested yet but are expected to work just fine. I can't say anything about compatibility with other mod managers.

Mod Organizer 1 is Not compatible, MO1 uses those same functions this plugin hooks to inject the BSA list into the INI settings and a bunch of other stuff. This plugin interferes with that making MO1 not work correctly. MO1 already has some optimizations in place for this problem so this mod is not really needed that much for MO1 users. 

**Q:** I installed it and game crashes at startup.  
**A:** Enable log in config file and reproduce your crash. Go to `Documents\My Games\<Game>\<xSE>` or `Data\<xSE>\Plugins`, look for `PrivateProfileRedirector.log` file, zip it if the file is too large, upload it somewhere and post a link to it in your issue report.

# Building
To build this project from sources you'll need:
- Visual Studio 2022.
- The source code for all supported xSE projects you want to build it for (NVSE is preset and builds, but isn't supported). Look at the project inlude paths to see where the build system is expecting to find the files.
- [KxFramework](https://github.com/Karandra/KxFramework) and [Microsoft Detours](https://github.com/microsoft/Detours) (included with KxFramework). Both can be installed through VCPkg.
