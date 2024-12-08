option(BUILD_APP5_SUPPORT "Build App#5 ACES support?" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
find_package(asdcplib REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(XercesC REQUIRED)
find_package(ZLIB REQUIRED)
find_package(regxmllib REQUIRED)
find_package(QtAppBase REQUIRED)

if(BUILD_APP5_SUPPORT)
	message(STATUS "Building with App#5 ACES support")
	find_package(OpenEXR REQUIRED)
	find_package(Imath REQUIRED)
endif ()

if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with Doxygen" VERBATIM)
	message(STATUS "Use target \"doc\" to generate doxygen.")
endif()

# Find includes in corresponding build directories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(qt_rcc_resources "${PROJECT_SOURCE_DIR}/resources/qt_resources.qrc")

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
	set(tool_src ${tool_src} As02AcesIStream.cpp ACES_Preview.cpp ACES_Decoder.cpp)
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
	set(tool_src ${tool_src} As02AcesIStream.h ACES_Preview.h ACES_Decoder.h)
endif(BUILD_APP5_SUPPORT)

set(synthesis_src synthesis/st0433-dcmlTypes.cpp synthesis/SMPTE-429-8-2006-PKL.cpp synthesis/SMPTE-429-9-2007-AM.cpp synthesis/SMPTE-2067-2-2013-Core.cpp 
	synthesis/SMPTE-2067-3-2013-CPL.cpp synthesis/SMPTE-2067-100a-2014-OPL.cpp synthesis/xml.cpp synthesis/xmldsig-core-schema.cpp
	synthesis/st2067-2a-2020-Core.cpp synthesis/st2067-2b-2020-PKL.cpp synthesis/st2067-3a-2020-CPL.cpp synthesis/SMPTE-2067-9a-2018-Sidecar.cpp
	synthesis/st2067-203a-20XX.cpp synthesis/st2067-204a-20XX.cpp
	)

source_group("Resource Files" FILES ${qt_rcc_resources} ${open_cl_src})
source_group("Synthesis Files" FILES ${synthesis_src})
qt_add_resources(resSources ${qt_rcc_resources})

include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/synthesis")

add_definitions(/DXSD_CXX11 /DXSD_CXX11_NULLPTR /DXSD_CXX11_TEMPLATE_ALIAS)
if(WIN32)
	add_definitions(/D_CRT_SECURE_NO_WARNINGS /DUNICODE /DKM_WIN32 /DASDCP_PLATFORM=\"win32\" /DNOMINMAX /DOPENEXR_DLL)
	if (MSVC)
		add_link_options(/SAFESEH:NO)
	endif ()
elseif(UNIX)
	add_definitions(/DASDCP_PLATFORM=\"unix\")
endif(WIN32)

if(BUILD_APP5_SUPPORT)
	add_definitions(/DAPP5_ACES)
endif(BUILD_APP5_SUPPORT)

qt_add_executable(IMF-Tool WIN32 MACOSX_BUNDLE ${tool_src} ${resSources} ${synthesis_src})

if(BUILD_APP5_SUPPORT)
	target_link_libraries(IMF-Tool
			PRIVATE Imath::Imath
			PRIVATE openexr::openexr
	)
endif(BUILD_APP5_SUPPORT)

target_link_libraries(IMF-Tool
		PRIVATE Qt6::Gui
		PRIVATE Qt6::Widgets
		PRIVATE Qt6::OpenGLWidgets
		PRIVATE QtAppBase
		PRIVATE ZLIB::ZLIB
		PRIVATE XercesC::XercesC
		PRIVATE regxmllib::regxmllib
		PRIVATE libasdcp
		PRIVATE libas02
		PRIVATE openjp2
	 )

register_icon(IMF-Tool
		ICO "${CMAKE_SOURCE_DIR}/resources/icon1.ico"
		ICNS "${CMAKE_SOURCE_DIR}/resources/IMFTool.icns"
		256x256 "${CMAKE_SOURCE_DIR}/resources/IMFTool256.png"
		48x48 "${CMAKE_SOURCE_DIR}/resources/IMFTool48.png"
)

install_app(IMF-Tool)
