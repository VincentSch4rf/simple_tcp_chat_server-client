cmake_minimum_required(VERSION 3.8)
project(chat_server)

set(CMAKE_C_STANDARD 99)

if(WIN32)
link_libraries(-lws2_32 -ladvapi32)
endif()
set(SOURCE_FILES server.c)
add_executable(chat_server ${SOURCE_FILES} server.c)