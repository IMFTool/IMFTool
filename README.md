# IMFTool
A tool for editing IMF CPLs and creating new versions of an existing IMF package

### Pre-compiled versions
The latest binary installers for macOS and Windows are available at
[Releases](../../releases/)

## What is IMF Tool
IMF Tool supports browsing and limited editing of IMF [[1]](#imf-intro) packages (IMPs).
In particular, it supports the addition and deletion of audio tracks and subtitle tracks (versioning).
Introductory videos are available on YouTube [[2]](#imf-video), [[3]](#imf-video2).

## Workflows supported:
-	Open an IMP, visualize the timeline of the CPL(s) included in the IMP
-	Supports App#2/2E and App#5 ACES
-	Track files support includes IAB (SMPTE ST 2067-201) and ISXD (SMPTE RDD 47)
-	Add Sidecar Assets to an IMP ("Add Asset" --> Add Sidecar Assets)
-	Create, view and edit Sidecar Composition Maps (SCMs)
-	Add a Photon QC report as sidecar file (hint: export the sidecar QC report as a Partial IMP, this leaves the Original IMP unmodified!)
-	Load ancestor Original Versions of Supplemental IMPs for preview and versioning
- 	Create Photon[[4]](#photon) QC report
-	Edit CPL metadata
-	Add new virtual tracks (limited to audio, subtitles and markers)
-	Import and wrap audio and timed text resources
-	Delete virtual tracks
-	Add new segments
-	Set edit points and markers
-	Export the IMP
-	Export all new MXF files and new CPL(s) as Partial IMP 
-	JPEG 2000 playback of all profiles supported in App #2 and App #2E for preview purposes
-	App#5 ACES support includes ACES preview and Target Frame preview and export
-	Decoding and rendering of IMSC1 text profile subtitles in a separate window. (TTML tab)
-	Overlay of IMSC1 image profile subtitles
-	Ingest of IMF 1.0 (PKL ST 429-8 and CPL ST 2067-3:2013) and IMF 1.1 (PKL ST 2067-2:2016 and CPL ST 2067-3:2016)
-	Outgest will be IMF 1.1 only
-	Editing of the ContentVersionList element
-	Edit Marker Annotations

## CREDITS
The development of this tool has kindly been sponsored by Netflix Inc.
The App#5 extensions were supported by the Academy of Motion Picture Arts and Sciences, Sony Pictures, Warner Bros., Universal Studios and 20th Century Fox

## What IMF Tool NOT is
An IMF Authoring Tool. For creating IMF packages, please check for the wide variety of commercial solutions available on the market.

## Binary installers
Please visit
[Releases](../../releases/)
to download the latest binary installers.

**For being notified about new releases, please "Watch" IMF Tool (on the top of this page).**


## Building
IMF Tool is multi-platform and has been successfully built under Mac OS X 10.10 and 10.11, Windows 7/10 and Linux 64 bit.
The build system is based on CMake. Please use CMake to create make files and project files for eclipse or Visual Studio. Installation instructions including CMake screenshots are provided as pdf file here.
Prerequisites:
-	Qt Version 5.12, more recent versions may work
-	NEW: For IAB and ProRes support, a patched version of asdcplib is required: https://github.com/wruppelx/asdcplib/tree/features.IAB.ProRes
-	libxsd
-	Xerces 3.1
-	Requires OpenJPEG 2.2 (with multi-threading support), available at https://github.com/uclouvain/openjpeg
-	regxmllibc, available at https://github.com/sandflow/regxmllib/

## DISCLAIMER
  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.



<a name="imf-intro"></a>[1] IMF: Interoperable Master Format. For an introduction see here:
http://techblog.netflix.com/2016/03/imf-prescription-for-versionitis.html

<a name="imf-video"></a>[2] IMF-Tool introductory video: https://www.youtube.com/watch?v=Zi3p8oElPM8

<a name="imf-video2"></a>[3] IMF-Tool introductory video Part 2: https://www.youtube.com/watch?v=k6OIx7WfF8s

<a name="photon"></a>[4] Photon IMF QC tool https://github.com/Netflix/photon

