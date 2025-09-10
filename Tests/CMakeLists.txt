if(BUILD_TESTS)

find_package(Qt6 REQUIRED COMPONENTS Test QuickTest Qml)

qt_add_executable(
  JASPTest
  ${SYSTEM_TYPE}
  testall.h
  testall.cpp
  $<$<PLATFORM_ID:Darwin>:${_R_Framework}>
  $<$<PLATFORM_ID:Windows>:${CMAKE_SOURCE_DIR}/Desktop/JASP.exe.manifest>
)

add_dependencies(JASPTest JASPDesktopLib)


target_include_directories(
  JASPTest
    PUBLIC
    JASPDesktopLib
)


target_link_libraries(
  JASPTest
	PUBLIC
    JASPDesktopLib
    Qt::Test
    Qt6::QuickTest
    Qt6::Qml
  )


#[[The test harness scans the specified source directory recursively
for "tst_*.qml" files. By default, it looks in the current directory,
which is usually where the executable is. This command makes it look
in the project's source directory instead.]]
add_definitions(-DQUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/TestLibrary/qml/")


enable_testing(true)

add_test(NAME testall COMMAND testDataImports)

endif()
