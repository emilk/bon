cmake_minimum_required(VERSION 2.8)
project(BON)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions("-std=c99")
endif()
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

add_library(libbon
	libbon/bon.c
	libbon/bon.h
	libbon/private.h
	libbon/crc32.c
	libbon/crc32.h
	libbon/read.c
	libbon/type.c
	libbon/write.c)
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

include_directories(${BON_SOURCE_DIR}/BON)
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

install(TARGETS
	libbon
	json2bon
	bon2json
	bon
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(FILES
	libbon/bon.h
	libbon/private.h
	libbon/inline.h
	libbon/read_inline.h
	libbon/write_inline.h
	DESTINATION include/bon)

install(FILES
	libbon/BON.1
	bon2json/bon2json.1
	json2bon/json2bon.1
	DESTINATION share/man)
