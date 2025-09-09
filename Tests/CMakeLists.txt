if(BUILD_TESTS)

find_package(Qt6 REQUIRED COMPONENTS Test)

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
  )



enable_testing(true)

add_test(NAME testall COMMAND testDataImports testMe)

add_custom_command(
  TARGET JASPTest
  POST_BUILD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory Tests/TestLibrary
          ${CMAKE_BINARY_DIR}/Testing/TestLibrary
  BYPRODUCTS ${CMAKE_BINARY_DIR}/Testing/TestLibrary
  COMMENT "------ Updating the Resources")

endif()
