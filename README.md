# Thorium Editor
The Editor for the Thorium Game Engine.

currently only windows is supported
# Building
Requirements
- [Thorium Engine source](https://github.com/ObsoleteJames/ThoriumEngine)
- CMake 3.20.0 or greater
- MSVC 15 (Visual Studio 2017) or greater for windows

### Windows
- Run 'build_editor_vsproj.bat'
- Open the Visual Studio project (in ThoriumEditor/Intermediate/build) and compile the editor
- In order to debug the editor, open the project properties and set 'Debugging->Working Directory' to 'PATH TO ENGINE REPOSITORY/engine/bin/win64'
