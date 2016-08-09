cmake_minimum_required(VERSION 2.6)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(qt4-mimetypes
          VERSION 1.0.1)
else()
  project(qt4-mimetypes)
  set(PROJECT_VERSION_MAJOR "1")
  set(PROJECT_VERSION_MINOR "0")
  set(PROJECT_VERSION_PATCH "1")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2016")
set(PROJECT_DOMAIN_FIRST "qt4-mimetypes")
set(PROJECT_DOMAIN_SECOND "com")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(Qt4MimetypesSetupQt)
include(PrependPath)

set(CMAKE_AUTOMOC ON)

add_definitions("-DBUILD_QT4_MIMETYPES")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/QtMimeTypes
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes/mime
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes/inqt5)
include_directories(${QT_MKSPECS_DIR}/default)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

# shared library
aux_source_directory(src/mimetypes MIMETYPES_SOURCES)
aux_source_directory(src/mimetypes/mime MIME_SOURCES)

set(SOURCES ${MIMETYPES_SOURCES} ${MIME_SOURCES})
prepend_path(SOURCES "${SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_win.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_mac.cpp)
else()
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_unix.cpp)
endif()

set(RESOURCES src/mimetypes/mimetypes.qrc)
qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${${PROJECT_NAME}_RESOURCES_RCC})
target_link_libraries(${PROJECT_NAME} ${QT_QTCORE_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION "${PROJECT_VERSION_MAJOR}")

if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/Zc:wchar_t- ${SHARED_FLAGS} -DQT_BUILD_INTERNAL -DQT_MAKEDLL")
elseif(MINGW)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_MAKEDLL ${SHARED_FLAGS}")
endif()

if(APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation -framework Carbon")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL QMIME_LIBRARY)

# if QtTest module was found, add tests
if(QT_QTTEST_LIBRARY)
  # qmimetype test
  aux_source_directory(tests/auto/qmimetype test_qmimetype_${PROJECT_NAME}_SOURCES)
  add_executable(test_qmimetype_${PROJECT_NAME} ${test_qmimetype_${PROJECT_NAME}_SOURCES})
  target_link_libraries(test_qmimetype_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
  add_test(test_qmimetype_${PROJECT_NAME} test_qmimetype_${PROJECT_NAME})

  # qmimedatabase-xml test
  add_definitions("-DCORE_SOURCES=\"${CMAKE_CURRENT_SOURCE_DIR}/src/\"")
  add_definitions("-DSRCDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/auto/qmimedatabase/\"")
  aux_source_directory(tests/auto/qmimedatabase/qmimedatabase-xml test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES)
  list(APPEND test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES tests/auto/qmimedatabase/tst_qmimedatabase.h)
  add_executable(test_qmimedatabase_xml_${PROJECT_NAME} ${test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES})
  target_link_libraries(test_qmimedatabase_xml_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
  add_test(test_qmimedatabase_xml_${PROJECT_NAME} test_qmimedatabase_xml_${PROJECT_NAME})

  if(MSVC OR MINGW)
    set_target_properties(test_qmimetype_${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_DLL")
    set_target_properties(test_qmimedatabase_xml_${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_DLL")
  endif()

  # qmimedatabase-cache test
  if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    aux_source_directory(tests/auto/qmimedatabase/qmimedatabase-cache test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES)
    list(APPEND test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES tests/auto/qmimedatabase/tst_qmimedatabase.h)
    add_executable(test_qmimedatabase_cache_${PROJECT_NAME} ${test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES})
    target_link_libraries(test_qmimedatabase_cache_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
    add_test(test_qmimedatabase_cache_${PROJECT_NAME} test_qmimedatabase_cache_${PROJECT_NAME})
  endif()
endif()

# installation settings
if(NOT CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(NOT CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if(NOT CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")
endif()

if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/CMake")
else()
  set(DEF_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/qt4-mimetypes")
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

if(MSVC)
  set(QT4MIMETYPES_LIBNAME_PREFIX "lib")
else()
  set(QT4MIMETYPES_LIBNAME_PREFIX "")
endif()

if(APPLE)
  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH  FALSE)

  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_LIBDIR})

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  # the RPATH to be used when installing, but only if it's not a system directory
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES ${CMAKE_INSTALL_LIBDIR} isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_LIBDIR})
  endif("${isSystemDir}" STREQUAL "-1")
endif()

configure_file(cmake/modules/qt4-mimetypesBuildTreeSettings.cmake.in
  ${PROJECT_BINARY_DIR}/qt4-mimetypesBuildTreeSettings.cmake @ONLY)
configure_file(cmake/modules/qt4-mimetypesConfig.cmake.in
  ${PROJECT_BINARY_DIR}/qt4-mimetypesConfig.cmake @ONLY)
configure_file(cmake/modules/qt4-mimetypesConfigVersion.cmake.in
  ${PROJECT_BINARY_DIR}/qt4-mimetypesConfigVersion.cmake @ONLY)

# install shared library
install(TARGETS ${PROJECT_NAME}
  EXPORT qt4-mimetypesLibraryDepends
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install cmake module
install(EXPORT qt4-mimetypesLibraryDepends DESTINATION ${INSTALL_CMAKE_DIR})
install(FILES ${PROJECT_BINARY_DIR}/qt4-mimetypesConfig.cmake DESTINATION ${INSTALL_CMAKE_DIR})
install(FILES ${PROJECT_BINARY_DIR}/qt4-mimetypesConfigVersion.cmake DESTINATION ${INSTALL_CMAKE_DIR})

# install headers
set(PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/QtMimeTypes/QMimeType)
list(APPEND PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/QtMimeTypes/QMimeDatabase)
list(APPEND PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/src/mimetypes/qmimetype.h)
list(APPEND PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/src/mimetypes/qmime_global.h)
list(APPEND PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/src/mimetypes/qmimedatabase.h)
foreach(ITEM ${PUBLIC_HEADERS})
  install(FILES ${ITEM} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt4-mimetypes)
endforeach()
