cmake_minimum_required (VERSION 2.8)
project (chunks)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Name of the executable
set(EXENAME ${CMAKE_PROJECT_NAME})

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include_directories(${PROJECT_SOURCE_DIR}/inc)

file(GLOB PROJECT_SOURCES src/*.c)
file(GLOB PROJECT_HEADERS inc/*.h)
					
add_executable(chunks ${PROJECT_SOURCES} ${PROJECT_INCLUDES})

