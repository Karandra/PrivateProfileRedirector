*Dates in DD.MM.YYYY*

# Version 0.2.0, 28.07.2018
- Added option "NativeWrite" that allows use of native "WritePrivateProfileString" function. Use it, if you experience write issues, like some files being written incorrectly.
- Added option "TrimKeyNamesA". It is required for some ENB presets, as they can request their settings with non-normalized key names, like "       CC: Gamma=1.50 " when "CC: Gamma=1.50" is expected.
- Added option "ANSICodePage". It allows to set code page to to convert non-ASCII characters to internal representation. It's not recommended to change the default value.
- For Special Edition version console command "RefreshINI" will reload INIs from disk in addition to its usual functionality.
