# IMFTool
A tool for editing IMF CPLs and creating new versions of an existing IMF package

### NEW
The latest binary installers are available at
[Releases](../../releases/)


## What is IMF Tool
IMF Tool supports browsing and limited editing of IMF [1] packages (IMPs).
In particular, it supports the addition and deletion of audio tracks and subtitle tracks (versioning).
Introductory videos are available on YouTube [2], [3].

## Workflows supported:
-	Open an IMP, visualize the timeline of the CPL(s) included in the IMP
-	Add new virtual tracks (limited to audio, subtitles and markers)
-	Import and wrap audio and timed text resources
-	Delete virtual tracks
-	Add new segments
-	Set edit points and markers
-	Export the IMP
-	Export all new MXF files and new CPL(s) as Partial IMP 
-	JPEG 2000 playback of all profiles supported in App #2 and App #2E for preview purposes
-	Decoding and rendering of IMSC1 text profile subtitles in a separate window. (TTML tab)
-	Overlay of IMSC1 image profile subtitles
-	Ingest of IMF 1.0 (PKL ST 429-8 and CPL ST 2067-3:2013) and IMF 1.1 (PKL ST 2067-2:2016 and CPL ST 2067-3:2016)
-	Outgest will be IMF 1.1 only
-	Editing of the ContentVersionList element

## CREDITS
The initial development of this tool has kindly been sponsored by Netflix Inc.

## What IMF Tool NOT is
An IMF Authoring Tool. For creating IMF packages, please check for the wide variety of commercial solutions available on the market.

## Binary installers
Please visit
[Releases](../../releases/)
to download the latest binary installers.

## Runtime Requirements
-	For creating essence descriptors, IMF Tool uses regxmllib [4] and requires Oracle JDK 1.8 to be installed, and java/java.exe being available in the executable search path. (In particular under Mac OS X, the JRE is not sufficient - you'll have to install the JDK !)

## Building
IMF Tool is multi-platform and has been susccesfully built under Mac OS X 10.10 and 10.11, Windows 7 and Linux 64 bit.
The build system is based on CMake. Please use CMake to create make files and project files for eclipse or Visual Studio. Installation instructions including CMake screenshots are provided as pdf file here.
Prerequisites:
-	Qt Version 5.x
-	asdcplib, see http://www.cinecert.com. IMPORTANT: A few files of asdcplib-2.x.xx need to be replaced by the files provided in folder asdcplib-2.x.xx-patches
-	libxsd
-	Xerces 3.1
-	Requires OpenJPEG 2.2 (with multi-threading support), available at https://github.com/uclouvain/openjpeg

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

[3] IMF-Tool introductory video Part 2: https://www.youtube.com/watch?v=k6OIx7WfF8s

[4] regxmllib: https://github.com/sandflow/regxmllib


