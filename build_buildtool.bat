set ENGINE_PATH_KEY=HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0
set ENGINE_PATH="NULL"

for /f "tokens=3" %%a in ('reg query HKEY_CURRENT_USER\SOFTWARE\ThoriumEngine\1.0 /v path /t REG_SZ  ^|findstr /ri "REG_SZ"') do set ENGINE_PATH=%%a

cmake -DENGINE_PATH="%ENGINE_PATH%" -B "BuildTool/Intermediate/build" "BuildTool"
cmake --build "BuildTool/Intermediate/build"