# PrivateProfileRedirector
Speeds up game start by storing INI files in memory instead of opening, parsing and closing the file each time some value from it is needed.

# Problem
The problem this plugin tries to solve comes from the fact that the game uses an old, deprecated and extremely inefficient function to load values from INI files. The function in question is `GetPrivateProfileString`, a relic of 16 bit operating systems. But do you know what is worse than using such a function? Using it hundreds of thousands of times.Apparently the game uses that function for every game setting individually, which means that the same ini file is opened and closed hundreds of times. What really makes this terrible is the fact that for each enabled plugin the game will attempt to read all the same settings from a hypothetical INI file that that plugin might or might not be loading. So you end up with an insane amount of incredibly inefficient calls for each enabled mod plugin. As a solution this mod will hook the functions involved and load the target INI files in memory so that each following call to the same INI file will be much faster as it will read directly from memory and will not require opening the file from scratch again.

The amount of seconds that this mod will reduce your start time by depends on the amount of enabled plugins you have, as well as other things using INI files such as ENB (though ENB comes with it's own loading issues that this mod can't fix). It will be basically imperceptible when used on the vanilla game and will go up from there, shaving off 6 seconds for 50 or so plugins, up to 20+ seconds for 250+ plugins.

# Solution
When the game, ENB or something else calls `GetPrivateProfileString` first time this plugin will load requested file in memory and return required data. Next time, no file will be loaded and data will be fetched from memory as well. Same thing happens when process tries to write a value to file. Instead of opening file again, parse and save to disk the plugin will write data it to in-memory file and then saves to disk (if allowed). Also all files is saved on game close, if game won't crash in process.


# Requirements
SKSE/SKSE64, [SKSE Plugin Preloader](https://www.nexusmods.com/skyrim/mods/75795) for LE/[SSE Engine Fixes](https://www.nexusmods.com/skyrimspecialedition/mods/17230) for SE.

# Covered functions
```
GetPrivateProfileStringA
GetPrivateProfileStringW
WritePrivateProfileStringA
WritePrivateProfileStringW
GetPrivateProfileIntA
GetPrivateProfileIntW
```

# Configuration
Plugin can be configures in its own INI file, each parameter is described inside this file.

# Q&A
**Q:** Do I really need this?

**A:** It depends. If your game starts from shortcut to main menu in split second then no. If it takes longer, this plugin can help. How much time you will save depends on your setup.

**Q:** Will this work with MO/NMM/Vortex/Kortex (yeah, I had to mention it) or any other mod manager?

**A:** It was tested with MO2 and Kortex and no problems was found. Vortex and NMM have not been tested yet but are expected to work just fine. I can't say anything about compatibility with other mod managers.

**Q:** I installed it and game crashes at startup.

**A:** Enable log in config file and reproduce your crash. Go to Data\SKSE\Plugins, look for file PrivateProfileRedirector.log, zip it, upload it somewhere and post a link to it in your issue report. Log file can be more that 100 MB, so don't upload it as is, be sure to compress it.
