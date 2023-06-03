cmake_minimum_required(VERSION 3.23)
project(tinyjvm C)

set(CMAKE_C_STANDARD 99)

include_directories(include)
add_library(java SHARED libjava/Object.c)
install(TARGETS java LIBRARY DESTINATION lib/tinyjvm)


add_library(native_test SHARED tests/data/native_test/NativeBasic.c)
install(TARGETS native_test LIBRARY DESTINATION lib/tinyjvm)

add_executable(test_loader
        jvm.h
        util.c
        class_loader.c
        util.h
        tests/test_loader.c
        jvm.c
        class_loader.h
        list.c
        list.h
        native.c
        native.h
        tests/test.h
        include/jni.h
        string.c
        string.h
        hashtable.c
        hashtable.h)

add_executable(test_interpreter
        jvm.h
        util.c
        class_loader.c
        util.h
        jvm.c
        class_loader.h
        list.c
        list.h
        native.c
        native.h
        tests/test.h
        include/jni.h
        string.c
        string.h
        hashtable.c
        hashtable.h
        tests/test_interpreter.c)
add_executable(test_hashtable
        tests/test_hashtable.c
        hashtable.h
        hashtable.c
        string.h
        string.c
        util.c
        util.h)
add_dependencies(test_interpreter java_lib)
#add_executable(tinyjvm jvm.h util.c class_loader.c util.h tests/test_loader.c)

target_compile_options(test_loader PRIVATE -fsanitize=address)
target_link_options(test_loader PRIVATE -fsanitize=address)

add_custom_target(java_lib
        COMMAND ./build.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)