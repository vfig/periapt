# Microsoft Developer Studio Project File - Name="lgLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# Hand-generated. Use at your own risk.

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lgLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lgLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lgLib.mak" CFG="lgLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lgLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lgLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lgLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "_WINDOWS" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "WIN32_LEAN_AND_MEAN" /D "WINVER=0x400" /D "_WIN32_WINNT=0x400" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lg.lib"

!ELSEIF  "$(CFG)" == "lgLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /D "WIN32_LEAN_AND_MEAN" /D "WINVER=0x400" /D "_WIN32_WINNT=0x400" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lg-d.lib"

!ENDIF 

# Begin Target

# Name "lgLib - Win32 Release"
# Name "lgLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;cxx;c;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\lg.cpp
# End Source File
# Begin Source File

SOURCE=.\scrmsgs.cpp
# End Source File
# Begin Source File

SOURCE=.\iids.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\lg\actreact.h
# End Source File
# Begin Source File

SOURCE=.\lg\ai.h
# End Source File
# Begin Source File

SOURCE=.\lg\config.h
# End Source File
# Begin Source File

SOURCE=.\lg\convict.h
# End Source File
# Begin Source File

SOURCE=.\lg\defs.h
# End Source File
# Begin Source File

SOURCE=.\lg\dynarray.h
# End Source File
# Begin Source File

SOURCE=.\lg\dynarray.hpp
# End Source File
# Begin Source File

SOURCE=.\lg\editor.h
# End Source File
# Begin Source File

SOURCE=.\lg\gen.h
# End Source File
# Begin Source File

SOURCE=.\lg\graphics.h
# End Source File
# Begin Source File

SOURCE=.\lg\iiddef.h
# End Source File
# Begin Source File

SOURCE=.\lg\iids.h
# End Source File
# Begin Source File

SOURCE=.\lg\input.h
# End Source File
# Begin Source File

SOURCE=.\lg\interface.h
# End Source File
# Begin Source File

SOURCE=.\lg\links.h
# End Source File
# Begin Source File

SOURCE=.\lg\malloc.h
# End Source File
# Begin Source File

SOURCE=.\lg\objects.h
# End Source File
# Begin Source File

SOURCE=.\lg\objstd.h
# End Source File
# Begin Source File

SOURCE=.\lg\propdefs.h
# End Source File
# Begin Source File

SOURCE=.\lg\properties.h
# End Source File
# Begin Source File

SOURCE=.\lg\quest.h
# End Source File
# Begin Source File

SOURCE=.\lg\res.h
# End Source File
# Begin Source File

SOURCE=.\lg\script.h
# End Source File
# Begin Source File

SOURCE=.\lg\scrmmanagers.h
# End Source File
# Begin Source File

SOURCE=.\lg\scrmsgs.h
# End Source File
# Begin Source File

SOURCE=.\lg\scrservices.h
# End Source File
# Begin Source File

SOURCE=.\lg\sound.h
# End Source File
# Begin Source File

SOURCE=.\lg\tools.h
# End Source File
# Begin Source File

SOURCE=.\lg\types.h
# End Source File
# Begin Source File

SOURCE=.\lg\win.h
# End Source File
# End Group
# End Target
# End Project
