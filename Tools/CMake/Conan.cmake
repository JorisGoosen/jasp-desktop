# Conan.cmake tries to run the `conan install` command using the right
# parameters. If everything goes right, you don't need to do anything,
# and CMake and Conan should handle all the dependencies properly. However,
# if you have any issues with Conan, you need to get your hand dirty, and
# acutally run the `conan install` command.
#
# In general, it's better if users are running their command, for now,
# if this works, I would like to handle it more automatically, but this
# turns out to be complicated or problematic, I will remove this and
# add a step to the build guide.
#
# As for what happens here, Conan download and build the necessary libraries,
# and if everything goes right, it generates several Find*.cmake files in the
# build folder, and these will be used by CMake and Libraries.cmake to find
# and link necessary libraries to JASP.

list(APPEND CMAKE_MESSAGE_CONTEXT Conan)

if(USE_CONAN)

  message(CHECK_START "Configuring Conan")

  execute_process(COMMAND_ECHO STDOUT WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND conan install ${CMAKE_SOURCE_DIR} -of ${CMAKE_BINARY_DIR})

endif()

list(POP_BACK CMAKE_MESSAGE_CONTEXT)
