# fbxInterop
# Note: This is currently in an unfinished, non-production state
---------------------
Setup
---
Required Libraries/Applications:
- Havok SDK 2014-1-0
- FBX SDK 2014.2.1
- Visual Studio 2012 for Platform Toolset V110

Required Environment Variables for installed libraries:
- HAVOK_SDK_ROOT
- FBX_SDK_ROOT

Compiling
---------------------
- 1.) Open solution file and make sure all libaries are installed
- 2.) Compile as Debug or Release
- 3.) Copy release/debug libfbxsdk.dll from FBX SDK to application directory

Usage
---------------------
- Run Godbert.
- Select a monster on the Monsters page.
- Click 'Export FBX'.

Credits
---------------------
- Autodesk, Inc. ( FBX SDK )
- Havok.com Inc. ( HAVOK Public SDK x32 )
- Ken Shoemake ( Euler/Quat Code Snippet )
- Figment ( Animation loading code idea/snippet from hkxcmd )
- Highflex ( Creator of havok2fbx tool )
- Rogueadyn ( hkAnimationInterop, Godbert )
- ufx ( more Godbert )
