# Host-side unit tests for Heimdall firmware logic (no ESP-IDF, no hardware).
#   cmake -S tests/host -B build-host && cmake --build build-host && ctest --test-dir build-host --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(heimdall_host_tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Werror)

include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0)
# SOURCE_SUBDIR points nowhere so only the sources are fetched (cJSON's own build is not needed).
FetchContent_Declare(cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG v1.7.19
    SOURCE_SUBDIR do-not-build)
FetchContent_MakeAvailable(unity cjson)

# cJSON as the firmware uses it (only the core file, warnings of third-party code relaxed).
add_library(cjson_host STATIC ${cjson_SOURCE_DIR}/cJSON.c)
target_include_directories(cjson_host PUBLIC ${cjson_SOURCE_DIR})
target_compile_options(cjson_host PRIVATE -Wno-error)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(FACE ${ROOT}/face/main)

enable_testing()

function(heimdall_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${FACE} ${ROOT}/main)
    target_link_libraries(${name} PRIVATE unity cjson_host m)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 10)
endfunction()

heimdall_test(test_face_proto test_face_proto.c ${FACE}/face_proto.c)
heimdall_test(test_face_expr test_face_expr.c ${FACE}/face_expr.c)
heimdall_test(test_touch_calib test_touch_calib.c ${FACE}/touch_calib.c)
heimdall_test(test_text_query test_text_query.cc)
