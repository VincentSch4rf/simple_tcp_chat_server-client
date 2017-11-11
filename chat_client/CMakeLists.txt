cmake_minimum_required(VERSION 3.8)
project(chat_client)

set(CMAKE_C_STANDARD 99)

if(WIN32)
link_libraries(-lws2_32 -ladvapi32)
endif()
set(SOURCE_FILES client.c)
add_executable(chat_client ${SOURCE_FILES} client.c)