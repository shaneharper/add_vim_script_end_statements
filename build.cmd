@echo off

if not exist %userprofile%\bin mkdir %userprofile%\bin
cl /c /std:c++17 /EHsc /TP /FI ciso646 with_end_statements.c++ ^
 && cl /std:c++17 /EHsc /TP /FI ciso646 test.c++ /link with_end_statements.obj ^
 && test.exe ^
 && cl /std:c++17 /EHsc /TP /FI ciso646 /Fe:%userprofile%\bin\add_vim_script_end_statements.exe main.c++ /link with_end_statements.obj

REM set PATH=%PATH%;%userprofile%\bin
