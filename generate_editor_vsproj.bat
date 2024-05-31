@echo off

set ENGINE_PATH_KEY=HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0
set ENGINE_PATH="NULL"

for /f "tokens=3" %%a in ('reg query HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0 /v path /t REG_SZ  ^|findstr /ri "REG_SZ"') do set ENGINE_PATH=%%a

%ENGINE_PATH%"/bin/win64/BuildTool.exe" "ThoriumEditor/Build.cfg" -development
cmake -DASSIMP_BUILD_TESTS=OFF -A x64 -B "ThoriumEditor/Intermediate/build" "ThoriumEditor/Intermediate"
pause