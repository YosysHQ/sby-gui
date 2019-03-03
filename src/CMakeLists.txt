set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

aux_source_directory(. GUI_SOURCE_FILES)
aux_source_directory(common COMMON_SOURCE_FILES)

qt5_add_resources(GUI_RESOURCE_FILES base.qrc)

add_executable(sby-gui ${COMMON_SOURCE_FILES} ${GUI_SOURCE_FILES} ${GUI_RESOURCE_FILES})
set_target_properties(sby-gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
target_include_directories(sby-gui PRIVATE common ../3rdparty/scintilla/qt/ScintillaEdit ../3rdparty/scintilla/qt/ScintillaEditBase ../3rdparty/scintilla/include)
target_compile_definitions(sby-gui PRIVATE QT_NO_KEYWORDS EXPORT_IMPORT_API=)
target_link_libraries(sby-gui LINK_PUBLIC Qt5::Widgets Qt5::Xml ScintillaEdit ${Boost_LIBRARIES})
install(TARGETS sby-gui RUNTIME DESTINATION bin)