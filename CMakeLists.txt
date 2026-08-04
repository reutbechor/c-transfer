cmake_minimum_required(VERSION 3.16)

project(c_transfer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(receiver src/receiver.c)
add_executable(sender src/sender.c)

target_compile_options(receiver PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(sender PRIVATE -Wall -Wextra -Wpedantic)