if(BUILD_TESTS)

find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(
  JASPTest
  ${SYSTEM_TYPE}
  testimport.h
  testimport.cpp
  $<$<PLATFORM_ID:Darwin>:${_R_Framework}>
  $<$<PLATFORM_ID:Windows>:${CMAKE_SOURCE_DIR}/Desktop/JASP.exe.manifest>
  main.cpp # Does this work?
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

endif()
