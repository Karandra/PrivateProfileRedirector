*Dates in DD.MM.YYYY*

# Version 0.5.2, 24.03.2021
- Updated to the latest xSE builds.
- Added experimental Fallout 4 VR build.

# Version 0.5.1, 22.03.2021
- Modified INI files are no longer written with double new lines.
- Primary location to the log file is moved to the `My Games\[xSE]` (`My Games\Skyrim Special Edition\SKSE`, `My Games\Fallout4\F4SE` and similar) folder inside the user's profile directory.

# Version 0.5, 25.01.2020
- Added 'SaveOnWriteBuffer' option to accumulate changes to the same INI file before saving to disk to avoid excessive IO when many options are changed at once.
- Added 'SaveOnProcessDetach' option to save changed files when game exits.
- Added 'SaveOnGameSave' option to allow save changed files before making saving the game.
- Fixed application of 'ProcessInlineComments' option. Now it should be applied for every INI value access.
- Console command `RefreshINI` now works in Skyrim LE.

# Version 0.4.1, 21.09.2019
- Fixed inline comments processing. Should fix issues with FreeFlyCam SKSE plugin (Skyrim LE/SE).
- Inline comments no longer being removed from the file on load.

# Version 0.4.0, 23.06.2019
- Fixed trimming quote characters for values and space characters for sections, keys and values. Should fix visual issues with some ENB presets.
- Removed options: `TrimKeyNamesA`, `TrimValueQuotes`, `ProcessByteOrderMark`, `DisableCCUnsafeA`.
- Proper locking for multithreaded read/write access.
- General refactoring.

# Version 0.3.5, 11.05.2019
- Fixed key names trimming when `TrimKeyNamesA` option is enabled. Should fix visual issues with some ENB presets.
- Added handling of empty key and value names in `GetPrivateProfileStringA` and `SetPrivateProfileStringA`.

# Version 0.3.4, 20.03.2019
- Added option `ProcessByteOrderMark` to support files encoded in UTF-16LE.

# Version 0.3.3, 09.01.2019
- Added option `AllowSEVersionMismatch` to initialize Script Extender functions even when runtime SE version does not match SE version for which this plugin was compiled.
- Removed file names conversion to lower case.

# Version 0.3.2, 04.10.2018
- Fixed possible crash when `TrimValueQuotes` option is enabled.
- KxDynamicString class is rewritten.

# Version 0.3.1, 12.08.2018
- Overridden new functions: `GetPrivateProfileSectionNamesA/W` and `GetPrivateProfileSectionA/W`.

# Version 0.3.0, 09.08.2018
- Added Fallout 4 version. Console command `RefreshINI` will work in Fallout 4.
- Added option `ProcessInlineComments` to discard inline comments, otherwise comments will be read as part of value.
- Added option `TrimValueQuotes` to correctly retrieve value like `sNegativeDir="ENBSeries\LUTs\Negative Stock"`.
- Fixed possible crash in `GetPrivateProfileString` when requested value is not found or its length is 0. Fixing this allows Creation Club to work which can be undesired. Option `DisableCCUnsafeA` were added to revert to old behavior.
- Function `GetPrivateProfileInt` can now parse integers in binary, octal and hex representations.

# Version 0.2.0, 28.07.2018
- Added option "NativeWrite" that allows use of native "WritePrivateProfileString" function. Use it, if you experience write issues, like some files being written incorrectly.
- Added option "TrimKeyNamesA". It is required for some ENB presets, as they can request their settings with non-normalized key names, like `       CC: Gamma=1.50 ` when `CC: Gamma=1.50` is expected.
- Added option "ANSICodePage". It allows to set code page to convert non-ASCII characters to internal representation. It's not recommended to change the default value.
- For Special Edition version console command `RefreshINI` will reload INIs from disk in addition to its usual functionality.
