cmake_minimum_required(VERSION 2.8)
project(BON)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions("-std=c99")
endif()

add_library(BON
	BON/bon.c
	BON/bon.h
	BON/bon_private.h
	BON/crc32.c
	BON/crc32.h
	BON/read.c
	BON/type.c
	BON/write.c)

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
target_link_libraries(json2bon BON jansson)

add_executable(bon2json
	bon2json/bon2json.c)
target_link_libraries(bon2json BON jansson)

install(TARGETS
	BON
	json2bon
	bon2json
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(FILES
	BON/bon.h
	DESTINATION include)

install(FILES
	BON/BON.1
	bon2json/bon2json.1
	json2bon/json2bon.1
	DESTINATION share/man)