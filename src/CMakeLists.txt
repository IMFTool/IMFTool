# Force decision if App#5 ACES support should be built
set(BUILD_APP5_SUPPORT true CACHE BOOL "Build App#5 ACES support? Run Generate again, if this option is being changed") 

find_file(asdcplib_Targets_PATH NAMES asdcplibtargets.cmake PATHS "$ENV{CMAKE_HINT}/asdcplib" ENV CMAKE_HINT PATH_SUFFIXES "targets")
find_path(asdcplib_include_DIR NAMES AS_02.h PATHS "$ENV{CMAKE_HINT}/asdcplib" ENV CMAKE_HINT PATH_SUFFIXES "include")
find_library(XercescppLib_PATH NAMES xerces-c xerces-c_3 PATHS "${PROJECT_SOURCE_DIR}/../xercescpp" "${PROJECT_SOURCE_DIR}/../lib/xercescpp" "$ENV{CMAKE_HINT}/xercescpp" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(XercescppLib_Debug_PATH NAMES xerces-cD xerces-c_3D xerces-c PATHS "${PROJECT_SOURCE_DIR}/../xercescpp" "${PROJECT_SOURCE_DIR}/../lib/xercescpp" "$ENV{CMAKE_HINT}/xercescpp" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_path(libz_include_DIR NAMES zlib.h zconf.h ENV CMAKE_HINT PATH_SUFFIXES "include" "zlib/include")
find_file(libz_PATH NAMES libz.a zlib.lib ENV CMAKE_HINT PATH_SUFFIXES "lib" "zlib/lib")
find_package(OpenJPEG)
find_package(regxmllibc)
find_path(XercescppLib_include_DIR NAMES xercesc/dom/DOM.hpp PATHS "${PROJECT_SOURCE_DIR}/../xercescpp" "${PROJECT_SOURCE_DIR}/../lib/xercescpp" "$ENV{CMAKE_HINT}/xercescpp" ENV CMAKE_HINT PATH_SUFFIXES "include")

if(BUILD_APP5_SUPPORT)
find_package(Imath CONFIG)
find_package(OpenEXR CONFIG)
find_library(OpenEXRLib_OpenEXR_PATH NAMES OpenEXR  PATHS "$ENV{CMAKE_HINT}/openexr" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(OpenEXRLib_OpenEXR_Debug_PATH NAMES OpenEXR  PATHS  "$ENV{CMAKE_HINT}/openexr" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_path(OpenEXRLib_include_DIR NAMES OpenEXR/ImfAttribute.h PATHS "${PROJECT_SOURCE_DIR}/../openexr" "${PROJECT_SOURCE_DIR}/../lib/openexr" "$ENV{CMAKE_HINT}/openexr" ENV CMAKE_HINT PATH_SUFFIXES "include")
find_path(ImathLib_include_DIR NAMES Imath/ImathVec.h PATHS "$ENV{CMAKE_HINT}/imath" ENV CMAKE_HINT PATH_SUFFIXES "include")
find_library(IlmBaseLib_Imath_PATH NAMES Imath Imath-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(IlmBaseLib_Imath_Debug_PATH NAMES Imath Imath-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(IlmBaseLib_IlmThread_PATH NAMES IlmThread IlmThread-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(IlmBaseLib_IlmThread_Debug_PATH NAMES IlmThread IlmThread-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(IlmBaseLib_Iex_PATH NAMES Iex Iex-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
find_library(IlmBaseLib_Iex_Debug_PATH NAMES Iex Iex-2_2 PATHS "${PROJECT_SOURCE_DIR}/../ilmbase" "${PROJECT_SOURCE_DIR}/../lib/ilmbase" "$ENV{CMAKE_HINT}/ilmbase" ENV CMAKE_HINT PATH_SUFFIXES "lib")
else(BUILD_APP5_SUPPORT)
unset(OpenEXRLib_OpenEXR_PATH CACHE)
unset(OpenEXRLib_OpenEXR_Debug_PATH CACHE)
unset(OpenEXRLib_include_DIR CACHE)
unset(ImathLib_include_DIR CACHE)
unset(IlmBaseLib_Imath_PATH CACHE)
unset(IlmBaseLib_Imath_Debug_PATH CACHE)
unset(IlmBaseLib_IlmThread_PATH CACHE)
unset(IlmBaseLib_IlmThread_Debug_PATH CACHE)
unset(IlmBaseLib_Iex_PATH CACHE)
unset(IlmBaseLib_Iex_Debug_PATH CACHE)
endif(BUILD_APP5_SUPPORT)

include("${asdcplib_Targets_PATH}") # Import asdcplib targets.

set (qt5_modules Core Gui Widgets)
find_package (Qt5 COMPONENTS ${qt5_modules})

if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with Doxygen" VERBATIM)
	message(STATUS "Use target \"doc\" to generate doxygen.")
endif(DOXYGEN_FOUND)

# Find includes in corresponding build directories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(qt_rcc_resources "${PROJECT_SOURCE_DIR}/resources/qt_resources.qrc")
set(win_resources "${PROJECT_SOURCE_DIR}/resources/win_resources.rc")

# source
set(tool_src main.cpp MainWindow.cpp KMQtLogSink.cpp QtWaitingSpinner.cpp WidgetAbout.cpp
	WidgetComposition.cpp ImfCommon.cpp WidgetImpBrowser.cpp ImfPackage.cpp ImfPackageCommon.cpp WizardWorkspaceLauncher.cpp 
	ImfPackageCommands.cpp MetadataExtractor.cpp MetadataExtractorCommon.cpp WizardResourceGenerator.cpp DelegateComboBox.cpp DelegateMetadata.cpp
	GraphicsWidgetTimeline.cpp GraphicsWidgetSegment.cpp CompositionPlaylistCommands.cpp ImfMimeData.cpp GraphicsCommon.cpp
	CustomProxyStyle.cpp GraphicScenes.cpp GraphicsWidgetResources.cpp GraphicsViewScaleable.cpp WidgetTrackDedails.cpp GraphicsWidgetComposition.cpp
	GraphicsWidgetSequence.cpp Events.cpp WidgetCentral.cpp 
	WidgetCompositionInfo.cpp UndoProxyModel.cpp JobQueue.cpp Jobs.cpp Error.cpp WizardPartialImpGenerator.cpp
	WidgetVideoPreview.cpp WidgetImagePreview.cpp JP2K_Preview.cpp JP2K_Decoder.cpp TTMLParser.cpp WidgetTimedTextPreview.cpp TimelineParser.cpp # (k)
	WidgetContentVersionList.cpp WidgetContentVersionListCommands.cpp WidgetLocaleList.cpp WidgetLocaleListCommands.cpp WizardEssenceDescriptor.cpp#WR
	WizardSidecarCompositionMapGenerator.cpp WizardCompositionGenerator.cpp PreviewCommon.cpp Player.cpp WidgetXmlTree.cpp WidgetSADMPreview.cpp XmlQSyntaxHighlighter.cpp
	qcompressor.cpp
	)
if(BUILD_APP5_SUPPORT)
set(tool_src ${tool_src} As02AcesIStream.cpp ACES_Preview.cpp ACES_Decoder.cpp
	)
endif(BUILD_APP5_SUPPORT)

# header
set(tool_src ${tool_src} global.h MainWindow.h KMQtLogSink.h QtWaitingSpinner.h 
	WidgetAbout.h WidgetComposition.h ImfCommon.h WidgetImpBrowser.h ImfPackage.h ImfPackageCommon.h WizardWorkspaceLauncher.h 
	ImfPackageCommands.h MetadataExtractor.h MetadataExtractorCommon.h WizardResourceGenerator.h DelegateComboBox.h DelegateMetadata.h 
	GraphicsWidgetTimeline.h GraphicsWidgetSegment.h CompositionPlaylistCommands.h ImfMimeData.h GraphicsCommon.h
	CustomProxyStyle.h GraphicScenes.h GraphicsWidgetResources.h GraphicsViewScaleable.h WidgetTrackDedails.h GraphicsWidgetComposition.h
	GraphicsWidgetSequence.h Events.h WidgetCentral.h Int24.h
	WidgetCompositionInfo.h UndoProxyModel.h SafeBool.h JobQueue.h Jobs.h Error.h WizardPartialImpGenerator.h
	WidgetVideoPreview.h WidgetImagePreview.h JP2K_Preview.h JP2K_Decoder.h TTMLParser.h WidgetTimedTextPreview.h TimelineParser.h SMPTE_Labels.h # (k)
	WidgetContentVersionList.h WidgetContentVersionListCommands.h WidgetLocaleList.h WidgetLocaleListCommands.h WizardEssenceDescriptor.h# WR
	WizardSidecarCompositionMapGenerator.h WizardCompositionGenerator.h PreviewCommon.h Player.h WidgetXmlTree.h WidgetSADMPreview.h XmlQSyntaxHighlighter.h
	qcompressor.h
	)
if(BUILD_APP5_SUPPORT)
set(tool_src ${tool_src} As02AcesIStream.h ACES_Preview.h ACES_Decoder.h
	)
endif(BUILD_APP5_SUPPORT)

set(synthesis_src synthesis/st0433-dcmlTypes.cpp synthesis/SMPTE-429-8-2006-PKL.cpp synthesis/SMPTE-429-9-2007-AM.cpp synthesis/SMPTE-2067-2-2013-Core.cpp 
	synthesis/SMPTE-2067-3-2013-CPL.cpp synthesis/SMPTE-2067-100a-2014-OPL.cpp synthesis/xml.cpp synthesis/xmldsig-core-schema.cpp
	synthesis/st2067-2a-2020-Core.cpp synthesis/st2067-2b-2020-PKL.cpp synthesis/st2067-3a-2020-CPL.cpp synthesis/SMPTE-2067-9a-2018-Sidecar.cpp
	synthesis/st2067-203a-20XX.cpp synthesis/st2067-204a-20XX.cpp
	)

source_group("Resource Files" FILES ${qt_rcc_resources} ${win_resources} ${open_cl_src})
source_group("Synthesis Files" FILES ${synthesis_src})
qt5_add_resources(resSources ${qt_rcc_resources})

include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/synthesis" "${XercescppLib_include_DIR}" 
	"${asdcplib_include_DIR}" "${OPENJPEG_INCLUDE_DIRS}" "${REGXMLLIBC_INCLUDE_DIRS}" "${libz_include_DIR}")

if(BUILD_APP5_SUPPORT)
include_directories("${OpenEXRLib_include_DIR}/OpenEXR" "${ImathLib_include_DIR}/Imath")
endif(BUILD_APP5_SUPPORT)

if(WIN32)
	add_definitions(/D_CRT_SECURE_NO_WARNINGS /DUNICODE /DKM_WIN32 /DASDCP_PLATFORM=\"win32\" /DNOMINMAX /DOPENEXR_DLL)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
elseif(UNIX)
	add_definitions(/DASDCP_PLATFORM=\"unix\")
endif(WIN32)

if(BUILD_APP5_SUPPORT)
add_definitions(/DAPP5_ACES)
endif(BUILD_APP5_SUPPORT)

add_executable(${EXE_NAME} WIN32 ${tool_src} ${resSources} ${win_resources} ${synthesis_src})
target_link_libraries(${EXE_NAME} general Qt5::Widgets
	 general libas02 debug "${XercescppLib_Debug_PATH}" optimized "${XercescppLib_PATH}" debug "${OpenJPEGLib_Debug_Path}" optimized "${OpenJPEGLib_Path}"
	 regxmllibc "${libz_PATH}" debug openjp2 optimized openjp2
	 )
if(BUILD_APP5_SUPPORT)
target_link_libraries(${EXE_NAME} 
	 debug "${IlmBaseLib_IlmThread_Debug_PATH}" optimized "${IlmBaseLib_IlmThread_PATH}" debug "${IlmBaseLib_Iex_Debug_PATH}" optimized "${IlmBaseLib_Iex_PATH}"
	 debug "${IlmBaseLib_Imath_Debug_PATH}" optimized "${IlmBaseLib_Imath_PATH}" debug "${OpenEXRLib_OpenEXR_Debug_PATH}" optimized "${OpenEXRLib_OpenEXR_PATH}"
	 )
endif(BUILD_APP5_SUPPORT)

# add the install target
install(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
