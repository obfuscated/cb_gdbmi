SET CB_SDK=%1\devel\share\codeblocks

if exist %CB_SDK%\debugger_gdbmi.zip del /f /q %CB_SDK%\debugger_gdbmi.zip
copy debugger_gdbmi.dll %CB_SDK%\debugger_gdbmi.dll
zip -j9 %CB_SDK%\debugger_gdbmi.zip resource\manifest.xml resource\*.xrc