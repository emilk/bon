//
//  main.cpp
//  test
//
//  Created by emilk on 2013-04-05.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

extern "C" {
#include "bon.h"
}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// File size (stat):
#include <sys/stat.h>
#include <sys/types.h>

#include <functional>




#define FILE_NAME "foo.bon"
#define N_VECS 4


uint8_t* readFile(size_t* out_size, const char* path)
{
	*out_size = 0;
	
	struct stat info;
	if (stat(path, &info) != 0) {
		fprintf(stderr, "Failed to stat file %s\n", path);
		return NULL;
	}
	
	size_t fileSize = (size_t)info.st_size;
	
	uint8_t* data = (uint8_t*)malloc(fileSize);
	
	FILE* fp = fopen(path, "rb");
	
	size_t blocks_read = fread(data, fileSize, 1, fp);
	if (blocks_read != 1) {
		fprintf(stderr, "Failed to read file %s\n", path);
		return NULL ;
	}
	
	fclose(fp);
	
	*out_size = fileSize;
	return data;
}


void writeStruct(bon_w_doc* doc)
{
	typedef struct {
		float    pos[3];
		float    normal[3];
		uint8_t  color[4];
	} Vertex;
	assert(sizeof(Vertex) == 3*sizeof(float) + 3*sizeof(float) + 4*sizeof(uint8_t));
	
	Vertex vecs[N_VECS];
	for (int i=0; i<N_VECS; ++i) {
		vecs[i].pos[0] = i;
		vecs[i].pos[1] = i;
		vecs[i].pos[2] = i;
		vecs[i].normal[0] = 100 + i;
		vecs[i].normal[1] = 100 + i;
		vecs[i].normal[2] = 100 + i;
		vecs[i].color[0] =  (uint8_t)i;
		vecs[i].color[1] =  (uint8_t)i;
		vecs[i].color[2] =  (uint8_t)i;
		vecs[i].color[3] =  (uint8_t)i;
	}
	
	bon_type*    type_vertex       =  bon_new_type_fmt("{[3f][3f][4u8]}", "pos", "normal", "color");
	bon_type*    type_vertex_list  =  bon_new_type_array(N_VECS, type_vertex);
	
	bon_w_aggregate(doc, type_vertex_list, vecs, sizeof(vecs));
	
	bon_free_type( type_vertex_list );
}

void readStruct(bon_r_doc* doc, const bon_value* val)
{
	/*
	 Ideally, one would simply spec up the receiving struct and
	 ask BON to to read all matching elements, doing conversions if necessary.
	 */
	
	typedef struct {
		float    pos[3];
		uint8_t  color[4];
	} Vertex;
	assert(sizeof(Vertex) == 3*sizeof(float) + 4*sizeof(uint8_t));
	
	bon_type*    type_vertex       =  bon_new_type_fmt("{[3f][4u8]}", "pos", "color");
	bon_type*    type_vertex_list  =  bon_new_type_array(N_VECS, type_vertex);
	
	/* Before reading, we want to inquire about the size of what we are about to read to */
	bon_size size = bon_r_list_size(val);
	
	printf("%d vecs: \n", (int)size);
	
	Vertex* vecs = (Vertex*)calloc(size, sizeof(Vertex));
	
#if 1
	if (bon_r_read_aggregate(doc, val, type_vertex_list, vecs, size * sizeof(Vertex))) {
		
		for (bon_size i=0; i<size; ++i) {
			printf("x: %.1f\n", vecs[i].pos[0]);
		}
	} else {
		fprintf(stderr, "Failed to read 'vecs'");
	}
#endif
}


void create()
{
	FILE* fp = fopen(FILE_NAME, "wb");
	
	bon_w_doc* doc = bon_w_new_doc(&bon_file_writer, fp);
	bon_w_header(doc);
	bon_w_begin_obj(doc);
	
#if 1
	bon_w_key(doc, "nil", BON_ZERO_ENDED);
	bon_w_nil(doc);
	
	bon_w_key(doc, "false", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_FALSE);
	
	bon_w_key(doc, "true", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	
	bon_w_key(doc, "uint8", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7f);
	
	bon_w_key(doc, "sint8", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x80);
	
	bon_w_key(doc, "uint16", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fff);
	
	bon_w_key(doc, "sint16", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x8000);
	
	bon_w_key(doc, "uint32", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fffffff);
	
	bon_w_key(doc, "sint32", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x80000000LL);
	
	bon_w_key(doc, "uint64", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fffffffffffffffULL);
	
	bon_w_key(doc, "sint64", BON_ZERO_ENDED);
	bon_w_sint64(doc, (int64_t)-0x8000000000000000LL);
	
#ifdef NAN
	bon_w_key(doc, "nan", BON_ZERO_ENDED);
	bon_w_float(doc, NAN);
#endif
	
#ifdef INFINITY
	bon_w_key(doc, "pos_inf", BON_ZERO_ENDED);
	bon_w_float(doc, +INFINITY);
	
	bon_w_key(doc, "neg_inf", BON_ZERO_ENDED);
	bon_w_float(doc, -INFINITY);
#endif
	
	bon_w_key(doc, "list", BON_ZERO_ENDED);
	bon_w_begin_list(doc);
	bon_w_uint64(doc, 2);
	bon_w_uint64(doc, 3);
	bon_w_uint64(doc, 5);
	bon_w_uint64(doc, 7);
	bon_w_string(doc, "eleven", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "nested", BON_ZERO_ENDED);
	bon_w_uint64(doc, 13);
	bon_w_end_obj(doc);
	bon_w_end_list(doc);
	
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "and", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "and", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_end_obj(doc);
	bon_w_end_obj(doc);
	bon_w_end_obj(doc);
#endif
	
#if 1
	bon_w_key(doc, "array", BON_ZERO_ENDED);
	double doubles[3] = {1.0f, 0.5f, 0.33333333333333333f};
	bon_w_array(doc, 3, BON_TYPE_FLOAT64, doubles, sizeof(doubles));
	
	bon_type*  type_vec3    =  bon_new_type_simple_array(3, BON_TYPE_FLOAT32);
	bon_type*  type_mat3x3  =  bon_new_type_array(3, type_vec3);
	
	float identity[3][3] = {{1,0,0}, {0,1,0}, {0,0,1}};
	
	bon_w_key(doc, "mat", BON_ZERO_ENDED);
	bon_w_aggregate(doc, type_mat3x3, identity, 3*3*sizeof(float));
#endif
	
	bon_w_key(doc, "vecs", BON_ZERO_ENDED);
	writeStruct(doc);
	
	bon_w_end_obj(doc);
	bon_w_close_doc(doc);
	
	fclose(fp);
	
#if 0
	bon_w_free_type(&type_mat3x3);
	bon_w_free_type(&type_vec3);
#endif
}



void read_bon()
{
	size_t size;
	const uint8_t* data = readFile(&size, FILE_NAME);
	bon_r_doc* doc = bon_r_open(data, size);
	
	const bon_value* root = bon_r_root(doc);
	const bon_value* vecs = bon_r_get_key(root, "vecs");
	readStruct(doc, vecs);
	
	bon_r_close(doc);
}

typedef std::function<void(bon_w_doc*)> Writer;
typedef std::function<void(const uint8_t*, bon_size)> Verifyer;
typedef std::function<void(bon_r_doc*)> Reader;


void test(Writer w, Verifyer v, Reader r)
{
	bon_byte_vec vec = {0,0,0};
	
	{
		bon_w_doc* bon = bon_w_new_doc(bon_vec_writer, &vec);
		w(bon);
		bon_w_close_doc( bon );
	}
	
	if (v)
	{
		v(vec.data, vec.size);
	}
	
	if (r)
	{
		bon_r_doc* bon = bon_r_open(vec.data, vec.size);
		r(bon);
		bon_r_close(bon);
	}
	
	free(vec.data);
}


TEST_CASE( "BON/control codes", "Testing values of control codes" ) {
	REQUIRE( BON_CTRL_BLOCK_REF  == '@' );
	REQUIRE( BON_CTRL_BLOCK_REF  == 0x40 );
	REQUIRE( BON_CTRL_STRING_VLQ == '`' );
	REQUIRE( BON_CTRL_STRING_VLQ == 0x60 );
	
	REQUIRE( BON_CTRL_LIST_BEGIN == 0x5B );
	REQUIRE( BON_CTRL_LIST_END == 0x5D );
	REQUIRE( BON_CTRL_OBJ_BEGIN == 0x7B );
	REQUIRE( BON_CTRL_OBJ_END == 0x7D );
	
	
	bon_ctrl codes[] = {
		BON_CTRL_BLOCK_REF,      BON_CTRL_STRING_VLQ,
		BON_CTRL_ARRAY_VLQ,      
		BON_CTRL_HEADER,         
		BON_CTRL_TUPLE_VLQ,      BON_CTRL_STRUCT_VLQ,
		BON_CTRL_BLOCK_BEGIN,    BON_CTRL_BLOCK_END,
		BON_CTRL_TRUE,           BON_CTRL_FALSE,
		BON_CTRL_NIL,            
		BON_CTRL_SINT8,          
		BON_CTRL_UINT8,          
		BON_CTRL_SINT16_LE,      BON_CTRL_SINT16_BE,
		BON_CTRL_UINT16_LE,      BON_CTRL_UINT16_BE,
		BON_CTRL_SINT32_LE,      BON_CTRL_SINT32_BE,
		BON_CTRL_UINT32_LE,      BON_CTRL_UINT32_BE,
		BON_CTRL_SINT64_LE,      BON_CTRL_SINT64_BE,
		BON_CTRL_UINT64_LE,      BON_CTRL_UINT64_BE,
		BON_CTRL_FLOAT32_LE,     BON_CTRL_FLOAT32_BE,
		BON_CTRL_FLOAT64_LE,     BON_CTRL_FLOAT64_BE,
		BON_CTRL_LIST_BEGIN,     BON_CTRL_LIST_END,  // 0x5B   0x5D
		BON_CTRL_OBJ_BEGIN,      BON_CTRL_OBJ_END,   // 0x7B   0x7D
	};
	
	for (auto c : codes) {
		CAPTURE( c );
		REQUIRE( 64 <= c   );
		REQUIRE( c  <= 128 );
	}
}


TEST_CASE( "BON/simple doc", "Write a simple doc" ) {
	test(
		  [](bon_w_doc* bon) {
			  bon_w_begin_obj(bon);
			  bon_w_key(bon, "k", BON_ZERO_ENDED);
			  bon_w_string(bon, "v", BON_ZERO_ENDED);
			  bon_w_end_obj(bon);
			  
			  REQUIRE( bon->error == BON_SUCCESS );
		  },
		  
		  [](const uint8_t* data, bon_size size) {
			  REQUIRE(size == 10); // TODO: 8 for compressed trings
			  
			  unsigned ix=0;
			  REQUIRE( data[ix++] == BON_CTRL_OBJ_BEGIN );
			  REQUIRE( data[ix++] == BON_CTRL_STRING_VLQ );
			  REQUIRE( data[ix++] == 1 );
			  REQUIRE( data[ix++] == 'k' );
			  REQUIRE( data[ix++] == 0 );
			  REQUIRE( data[ix++] == BON_CTRL_STRING_VLQ );
			  REQUIRE( data[ix++] == 1 );
			  REQUIRE( data[ix++] == 'v' );
			  REQUIRE( data[ix++] == 0 );
			  REQUIRE( data[ix++] == BON_CTRL_OBJ_END );
			  REQUIRE( ix == size );
		  },
		  
		  [](bon_r_doc* bon) {
			  REQUIRE( bon->error == BON_SUCCESS );
			  
			  auto root = bon_r_get_block(bon, 0);
			  REQUIRE( root );
			  auto val = bon_r_get_key(root, "k");
			  REQUIRE( val );
			  REQUIRE( val->type == BON_VALUE_STRING );
			  REQUIRE( val->u.str.size == 1 );
			  REQUIRE( (const char*)val->u.str.ptr == std::string("v") );
		  }
	);
}
