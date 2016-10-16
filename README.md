# IMFTool
A tool for editing IMF CPLs and creating new versions of an existing IMF package

##What is IMF Tool
IMF Tool supports browsing and limited editing of IMF [1] packages (IMPs).
In particular, it supports the addition and deletion of audio tracks and subtitle tracks (versioning).
An introductory video is available on YouTube [2].

##Workflows supported:
-	Open an IMP, visualize the timeline of the CPL(s) included in the IMP
-	Add new virtual tracks (limited to audio, subtitles and markers)
-	Import and wrap audio and timed text resources
-	Delete virtual tracks
-	Add new segments
-	Set edit points and markers
-	Export the IMP
-	Export all new MXF files and new CPL(s) as Partial IMP 

##Workflows supported soon (under development):
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
For your convenience, we provide binary installers, currently for Mac OS and Windows, in the dist-binaries/ folder.
Please click on the installer image you would like to download and select "Download" on the page that opens.
Please carefully read dist-binaries/README.binaries.

##Runtime Requirements
For creating essence descriptors, IMF Tool uses regxmllib [3] and requires Oracle JDK 1.8 to be installed, and java/java.exe being available in the executable search path. (In particular under Mac OS X, the JRE is not sufficient - you'll have to install the JDK !)

##Building
IMF Tool is multi-platform and been susccesfully tested to build under Mac OS X 10.10 and 10.11, Windows 7 and Linux 64 bit.
The build system is based on CMake. Please use CMake to create make files and project files for eclipse or Visual Studio. Installation instructions including CMake screenshots are provided as pdf file here.
Prerequisites:
-	Qt Version 5.x
-	asdcplib, see http://www.cinecert.com. IMPORTANT: A few files of asdcplib-2.5.14 need to be replaced by the files provided in folder asdcplib-2.5.14-patches
-	libxsd
-	Xerces 3.1

##DISCLAIMER
  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.



[1] IMF: Interoperable Master Format. For an introduction see here:
http://techblog.netflix.com/2016/03/imf-prescription-for-versionitis.html

[2] IMF-Tool introductory video: https://www.youtube.com/watch?v=Zi3p8oElPM8

[3] regxmllib: https://github.com/sandflow/regxmllib


