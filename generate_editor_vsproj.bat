@echo off

set ENGINE_PATH_KEY=HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0
set ENGINE_PATH="NULL"

for /f "tokens=3" %%a in ('reg query HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0 /v path /t REG_SZ  ^|findstr /ri "REG_SZ"') do set ENGINE_PATH=%%a

"BuildTool/Intermediate/build/Debug/BuildTool.exe" "ThoriumEditor/Build.cfg"
cmake -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 17 2022" -A x64 -B "ThoriumEditor/Intermediate/build" "ThoriumEditor/Intermediate"
pause
