*Dates in DD.MM.YYYY*

# Version 0.3.2, 04.10.2018
- Fixed possible crash when `TrimValueQuotes` option is enabled.
- KxDynamicString class is rewritten.

# Version 0.3.1, 12.08.2018
- Overrided new functions: `GetPrivateProfileSectionNamesA/W` and `GetPrivateProfileSectionA/W`.

# Version 0.3.0, 09.08.2018
- Added Fallout 4 version. Console command `RefreshINI` will work in Fallout 4.
- Added option `ProcessInlineComments` to discard inline comments, otherwise comments will be read as part of value.
- Added option `TrimValueQuotes` to correctly retrieve value like `sNegativeDir="enbseries\LUTs\Negative Stock"`.
- Fiexd possible crash in `GetPrivateProfileString` when requested value is not found or its length is 0. Fixing this allows Creation Clud to work which can be undesired. Option `DisableCCUnsafeA` were added to revert to old behavior.
- Function `GetPrivateProfileInt` can now parse integers in binary, octal and hex representations.

# Version 0.2.0, 28.07.2018
- Added option "NativeWrite" that allows use of native "WritePrivateProfileString" function. Use it, if you experience write issues, like some files being written incorrectly.
- Added option "TrimKeyNamesA". It is required for some ENB presets, as they can request their settings with non-normalized key names, like "       CC: Gamma=1.50 " when "CC: Gamma=1.50" is expected.
- Added option "ANSICodePage". It allows to set code page to to convert non-ASCII characters to internal representation. It's not recommended to change the default value.
- For Special Edition version console command `RefreshINI` will reload INIs from disk in addition to its usual functionality.
