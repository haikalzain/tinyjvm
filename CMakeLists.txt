cmake_minimum_required(VERSION 3.23)
project(tinyjvm C)

set(CMAKE_C_STANDARD 99)




add_executable(test_loader jvm.h util.c class_loader.c util.h tests/test_loader.c jvm.c class_loader.h list.c list.h)
add_executable(tinyjvm jvm.h util.c class_loader.c util.h tests/test_loader.c)

target_compile_options(test_loader PRIVATE -fsanitize=address)
target_link_options(test_loader PRIVATE -fsanitize=address)
