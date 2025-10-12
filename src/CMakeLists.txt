set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

aux_source_directory(. GUI_SOURCE_FILES)
aux_source_directory(lexers LEXERS_SOURCE_FILES)

if (Qt6_FOUND)
    qt6_add_resources(GUI_RESOURCE_FILES base.qrc)
else()
    qt5_add_resources(GUI_RESOURCE_FILES base.qrc)
endif()

add_executable(sby-gui ${LEXERS_SOURCE_FILES} ${GUI_SOURCE_FILES} ${GUI_RESOURCE_FILES})
set_target_properties(sby-gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
target_include_directories(sby-gui PRIVATE common ../3rdparty/QScintilla/src ../3rdparty/QScintilla/scintilla/include ../3rdparty/QScintilla/scintilla/lexlib)
target_compile_definitions(sby-gui PRIVATE EXPORT_IMPORT_API=)
target_link_libraries(sby-gui LINK_PUBLIC ${QT_LIBRARIES} QScintilla)
install(TARGETS sby-gui RUNTIME DESTINATION bin)