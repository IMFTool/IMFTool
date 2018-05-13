This folder contains patches (file replacements) for asdcplib-2.7.19
asdcplib can be downloaded from www.cinecert.com

# Instructions (READ CAREFULLY):

* Download and unpack from www.cincert.com

* Attention: There are Two CMakeLists.txt files to be replaced, on in base folder <asdcplib>/, one in the <asdcplib>/src/ subfolder!

* Copy CMakeLists.txt into the <asdcplib>/ folder.

* Copy the files in directory src/ into the <asdcplib>/src/ folder.

* Follow the build instructions in README.cmake in the root folder of the asdcplib distribution

* Install asdcplib using make install (Linux, macOS) or INSTALL target in Visual Studio

# List of patch files fixing minor conformance issues:
File | Modifcations
----------- | -------------
 ./src/AS_02_JP2K.cpp | Set EditRate in Index Table (was always 0/0 before)
 ./src/AS_02_internal.h | Method `SetEditRate` added to class `AS02IndexWriterVBR`
 ./src/Metadata.cpp | Encodes `SoundEssenceCoding` and `DataEssenceCoding` only if they have a non-zero value
 ./src/h__02_Writer.cpp | Added `MinorVersion = 3` to `AS02IndexWriterVBR` and `AS02IndexWriterCBR` constructors

# List of further patches specific to IMF Tool
File | Modifcations
----------- | -------------
 ./CMakeLists.txt | Version detection, INSTALL target configuration
 ./src/AS_02_TimedText.cpp | Contains an extension to read the essence element of TTML MXF files with broken indes table. This replacement is optional.
 ./src/CMakeLists.txt | Modified CMake file solving Windows Visual Studio build issues.


# Disclaimer
All patches are provided "as-is". Please test carefully if they are appropriate for your environment.

asdcplib is (c) John Hurst, portions (c) Robert Scheler, Heiko Sparenberg Fraunhofer IIS, and John Hurst

