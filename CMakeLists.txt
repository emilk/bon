cmake_minimum_required(VERSION 2.8)
project(BON)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif()
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

add_library(libbon
	libbon/bon/bon.c
	libbon/bon/bon.h
	libbon/bon/crc32.c
	libbon/bon/crc32.h
	libbon/bon/inline.h
	libbon/bon/private.h
	libbon/bon/read.c
	libbon/bon/read_inline.h
	libbon/bon/type.c
	libbon/bon/write.c
	libbon/bon/write_inline.h)
SET_TARGET_PROPERTIES(libbon
  PROPERTIES OUTPUT_NAME bon)

add_library(jansson STATIC
	jansson/dump.c
	jansson/error.c
	jansson/hashtable.c
	jansson/hashtable.h
	jansson/jansson_config.h
	jansson/jansson.h
	jansson/jansson_private.h
	jansson/load.c
	jansson/memory.c
	jansson/pack_unpack.c
	jansson/strbuffer.c
	jansson/strbuffer.h
	jansson/strconv.c
	jansson/utf.c
	jansson/utf.h
	jansson/value.c)

include_directories(${BON_SOURCE_DIR}/libbon)
include_directories(${BON_SOURCE_DIR}/jansson)

add_executable(json2bon
	json2bon/json2bon.c)
target_link_libraries(json2bon libbon jansson)

add_executable(bon2json
	bon2json/bon2json.c)
target_link_libraries(bon2json libbon jansson)

add_executable(bon
	bon/bon.c)
target_link_libraries(bon libbon)
 
add_executable(test-core
	test/test.cpp)
target_link_libraries(test-core libbon)
 
add_test(core bin/test-core)

install(TARGETS
	libbon
	json2bon
	bon2json
	bon
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(FILES
	libbon/bon/bon.h
	libbon/bon/crc32.h
	libbon/bon/private.h
	libbon/bon/inline.h
	libbon/bon/read_inline.h
	libbon/bon/write_inline.h
	DESTINATION include/bon)

