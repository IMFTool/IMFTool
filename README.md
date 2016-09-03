# IMFTool
A tool for editing IMF CPLs and creating new versions of an existing IMF package

##What is IMF Tool
IMF Tool supports browsing and limited editing of IMF [1] packages (IMPs).
In particular, it supports the addition and deletion of audio tracks and subtitle tracks (versioning).

##Workflows supported:
-	Open an IMP, visualize the timeline of the CPL(s) included in the IMP
-	Add new virtual tracks (limited to audio, subtitles and markers)
-	Import and wrap audio and timed text resources
-	Delete virtual tracks
-	Add new segments
-	Set edit points and markers
-	Export the IMP

##Workflows supported soon (under development):
-	Export of the modifications as Partial (aka Supplemental) IMP
-	Editing of the ContentVersionList element, including custom LabelText entries
-	Audio Inserts

##CREDITS
The initial development of this tool has kindly been sponsored by Netflix Inc.

##What IMF Tool NOT is
An IMF Authoring Tool. For creating IMF packages, please check for the wide variety of commercial solutions available on the market.

##Limitations
Currently, IMF Tool does not provide a video preview. 
We are working on a solution.

##Binary installers
We provide binary installers, currently for Mac OS and Windows, in the dist/ folder.

##Runtime Requirements
For creating essence descriptors, IMF Tool uses regxmllib [2] and requires Oracle JRE 1.8 to be installed, and java/java.exe being available in the executable search path.

##Building
IMF Tool is multi-platform and been susccesfully tested to build under Mac OS X 10.10 and 10.11, Windows 7 and Linux 64 bit.
The build system is based on CMake. Please use CMake to create make files and project files for eclipse or Visual Studio. Installation instructions including CMake screenshots are provided as pdf file here.
Prerequisites:
-	Qt Version 5.x
-	asdcplib, see http://www.cinecert.com
-	libxsd
-	Xerces 3.1


[1] IMF: Interoperable Master Format. For an introduction see here:
http://techblog.netflix.com/2016/03/imf-prescription-for-versionitis.html
[2] regxmllib: https://github.com/sandflow/regxmllib



