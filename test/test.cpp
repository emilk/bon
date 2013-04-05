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


using namespace std;


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


void writeStruct(bon_w_doc* bon)
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
	
	bon_w_aggregate(bon, type_vertex_list, vecs, sizeof(vecs));
	
	bon_free_type( type_vertex_list );
}

void readStruct(bon_r_doc* bon, const bon_value* val)
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
	if (bon_r_read_aggregate(bon, val, type_vertex_list, vecs, size * sizeof(Vertex))) {
		
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
	
	bon_w_doc* bon = bon_w_new_doc(&bon_file_writer, fp, BON_FLAG_DEFAULT);
	bon_w_header(bon);
	bon_w_begin_obj(bon);
	
#if 1
	bon_w_key(bon, "nil", BON_ZERO_ENDED);
	bon_w_nil(bon);
	
	bon_w_key(bon, "false", BON_ZERO_ENDED);
	bon_w_bool(bon, BON_FALSE);
	
	bon_w_key(bon, "true", BON_ZERO_ENDED);
	bon_w_bool(bon, BON_TRUE);
	
	bon_w_key(bon, "uint8", BON_ZERO_ENDED);
	bon_w_uint64(bon, 0x7f);
	
	bon_w_key(bon, "sint8", BON_ZERO_ENDED);
	bon_w_sint64(bon, -0x80);
	
	bon_w_key(bon, "uint16", BON_ZERO_ENDED);
	bon_w_uint64(bon, 0x7fff);
	
	bon_w_key(bon, "sint16", BON_ZERO_ENDED);
	bon_w_sint64(bon, -0x8000);
	
	bon_w_key(bon, "uint32", BON_ZERO_ENDED);
	bon_w_uint64(bon, 0x7fffffff);
	
	bon_w_key(bon, "sint32", BON_ZERO_ENDED);
	bon_w_sint64(bon, -0x80000000LL);
	
	bon_w_key(bon, "uint64", BON_ZERO_ENDED);
	bon_w_uint64(bon, 0x7fffffffffffffffULL);
	
	bon_w_key(bon, "sint64", BON_ZERO_ENDED);
	bon_w_sint64(bon, (int64_t)-0x8000000000000000LL);
	
#ifdef NAN
	bon_w_key(bon, "nan", BON_ZERO_ENDED);
	bon_w_float(bon, NAN);
#endif
	
#ifdef INFINITY
	bon_w_key(bon, "pos_inf", BON_ZERO_ENDED);
	bon_w_float(bon, +INFINITY);
	
	bon_w_key(bon, "neg_inf", BON_ZERO_ENDED);
	bon_w_float(bon, -INFINITY);
#endif
	
	bon_w_key(bon, "list", BON_ZERO_ENDED);
	bon_w_begin_list(bon);
	bon_w_uint64(bon, 2);
	bon_w_uint64(bon, 3);
	bon_w_uint64(bon, 5);
	bon_w_uint64(bon, 7);
	bon_w_string(bon, "eleven", BON_ZERO_ENDED);
	bon_w_begin_obj(bon);
	bon_w_key(bon, "nested", BON_ZERO_ENDED);
	bon_w_uint64(bon, 13);
	bon_w_end_obj(bon);
	bon_w_end_list(bon);
	
	bon_w_key(bon, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(bon);
	bon_w_key(bon, "and", BON_ZERO_ENDED);
	bon_w_bool(bon, BON_TRUE);
	bon_w_key(bon, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(bon);
	bon_w_key(bon, "and", BON_ZERO_ENDED);
	bon_w_bool(bon, BON_TRUE);
	bon_w_key(bon, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(bon);
	bon_w_end_obj(bon);
	bon_w_end_obj(bon);
	bon_w_end_obj(bon);
#endif
	
#if 1
	bon_w_key(bon, "array", BON_ZERO_ENDED);
	double doubles[3] = {1.0f, 0.5f, 0.33333333333333333f};
	bon_w_array(bon, 3, BON_TYPE_FLOAT64, doubles, sizeof(doubles));
	
	bon_type*  type_vec3    =  bon_new_type_simple_array(3, BON_TYPE_FLOAT32);
	bon_type*  type_mat3x3  =  bon_new_type_array(3, type_vec3);
	
	float identity[3][3] = {{1,0,0}, {0,1,0}, {0,0,1}};
	
	bon_w_key(bon, "mat", BON_ZERO_ENDED);
	bon_w_aggregate(bon, type_mat3x3, identity, 3*3*sizeof(float));
#endif
	
	bon_w_key(bon, "vecs", BON_ZERO_ENDED);
	writeStruct(bon);
	
	bon_w_end_obj(bon);
	bon_w_close_doc(bon);
	
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
	bon_r_doc* bon = bon_r_open(data, size);
	
	const bon_value* root = bon_r_root(bon);
	const bon_value* vecs = bon_r_get_key(root, "vecs");
	readStruct(bon, vecs);
	
	bon_r_close(bon);
}

typedef std::function<void(bon_w_doc*)> Writer;
typedef std::function<void(const uint8_t*, bon_size)> Verifyer;
typedef std::function<void(bon_r_doc*)> Reader;


void test(Writer w, Verifyer v, Reader r)
{
	bon_byte_vec vec = {0,0,0};
	
	{
		bon_w_doc* bon = bon_w_new_doc(bon_vec_writer, &vec, BON_FLAG_DEFAULT);
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

TEST_CASE( "BON/code ranges", "Code ranges" ) {
	REQUIRE((BON_SHORT_NEG_INT_START + 16)  == 256);
}


TEST_CASE( "BON/control codes", "control codes" ) {
	REQUIRE( BON_CTRL_BLOCK_REF   == '@' );
	REQUIRE( BON_CTRL_BLOCK_REF   == 0x40 );
	REQUIRE( BON_CTRL_STRING_VLQ  == '`' );
	REQUIRE( BON_CTRL_STRING_VLQ  == 0x60 );
	
	REQUIRE( BON_CTRL_LIST_BEGIN  == 0x5B );
	REQUIRE( BON_CTRL_LIST_END    == 0x5D );
	REQUIRE( BON_CTRL_OBJ_BEGIN   == 0x7B );
	REQUIRE( BON_CTRL_OBJ_END     == 0x7D );
	
	
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

typedef std::vector<uint8_t> Bytes;

void append(Bytes& out, bon_ctrl v) {
	out.push_back( v );
}

void append(Bytes& out, int8_t v) {
	out.push_back( (uint8_t)v );
}

void append(Bytes& out, uint8_t v) {
	out.push_back( v );
}

void append(Bytes& out, char v) {
	out.push_back( (uint8_t)v );
}

void append(Bytes& out, unsigned long v) {
	assert(v < 256);
	out.push_back( (uint8_t)v );
}

void append(Bytes& out, unsigned v) {
	append(out, (unsigned long)v);
}

void append(Bytes& out, long v) {
	if (v < 0) {
		assert(-128 <= v);
		append(out, (int8_t)v);
	} else {
		assert(0 <= v);
		assert(v < 256);
		out.push_back( (uint8_t)v );
	}
}

void append(Bytes& out, int v) {
	append(out, (long)v);
}

void append(Bytes& out, const char* str) {
	while (*str) {
		append( out, *str );
		++str;
	}
}

void append(Bytes& out, const string& str) {
	for (char c : str) {
		append(out, c);
	}
}

template<typename T>
void appendOne(Bytes& out, const T& t) {
	append(out, t);
}

void appendAll(Bytes&) {
}

template<typename Head, typename... Tail>
void appendAll(Bytes& out, Head head, Tail... tail)
{
	appendOne(out, head);
	appendAll(out, tail...);
}

template<typename... Args>
Bytes bytes(Args... args)
{
	Bytes b;
	appendAll(b, args...);
	return b;
}


void require_identical(const uint8_t* data, bon_size size, const Bytes& expected)
{
	REQUIRE(size == expected.size());
	
	for (unsigned ix=0; ix<size; ++ix) {
		CAPTURE( ix );
		REQUIRE( data[ix] == expected[ix] );
	}
}


TEST_CASE( "BON/types", "Writes, verifies and reads all the basic types of BON" ) {
	
#if !__LITTLE_ENDIAN__
#  error "This test is designed for a little-endian machine. A big endian machine would output other control codes, and the integer bytes in reversed order."
#endif
	
	string STR_VLQ_1 = "0123456789012345678901234567890123456789"; // 40
	string STR_VLQ_2 =
	"01234567890123456789012345678901234567890123456789"
	"01234567890123456789012345678901234567890123456789"
	"01234567890123456789012345678901234567890123456789"; // 150
	
	test(
		  // Write 
		  [=](bon_w_doc* bon) {
			  bon_w_header(bon);
			  
			  bon_w_begin_obj(bon);
			  
			  bon_w_key(bon, "nil", BON_ZERO_ENDED);     bon_w_nil(bon);
			  bon_w_key(bon, "true", BON_ZERO_ENDED);    bon_w_bool(bon, BON_TRUE);
			  bon_w_key(bon, "seven", BON_ZERO_ENDED);   bon_w_sint64(bon, 7);
			  bon_w_key(bon, "neg_1", BON_ZERO_ENDED);   bon_w_sint64(bon, -1);
			  bon_w_key(bon, "42", BON_ZERO_ENDED);      bon_w_sint64(bon, 42);
			  bon_w_key(bon, "-0x80", BON_ZERO_ENDED);   bon_w_sint64(bon, -0x80);
			  bon_w_key(bon, "0x7fff", BON_ZERO_ENDED);  bon_w_sint64(bon, 0x7fff);
			  
			  //bon_w_key(bon, "f_pi", BON_ZERO_ENDED);  bon_w_float(bon, 3.14f);
			  //bon_w_key(bon, "f_di", BON_ZERO_ENDED);  bon_w_double(bon, 3.14f);
			  		  
			  
			  bon_w_key(bon, "a", BON_ZERO_ENDED);
			  bon_w_string(bon, STR_VLQ_1.c_str(), BON_ZERO_ENDED);
			  
			  bon_w_key(bon, "b", BON_ZERO_ENDED);
			  bon_w_string(bon, STR_VLQ_2.c_str(), BON_ZERO_ENDED);
			  
			  bon_w_end_obj(bon);
			  
			  REQUIRE( bon->error == BON_SUCCESS );
		  },
		  
		  
		  [=](const uint8_t* data, bon_size size) {
			  auto expected = bytes("BON0",
											BON_CTRL_OBJ_BEGIN,
											
											BON_SHORT_STRING(3), "nil", 0,     BON_CTRL_NIL,                                  
											BON_SHORT_STRING(4), "true", 0,    BON_CTRL_TRUE,                                 
											BON_SHORT_STRING(5), "seven", 0,   7,                                             
											BON_SHORT_STRING(5), "neg_1", 0,   0xff,
											BON_SHORT_STRING(2), "42", 0,      BON_CTRL_UINT8, 42,
											BON_SHORT_STRING(5), "-0x80", 0,   BON_CTRL_SINT8, -0x80,
											BON_SHORT_STRING(6), "0x7fff", 0,  BON_CTRL_UINT16, 0xff, 0x7f,
											
											
											BON_SHORT_STRING(1), 'a', 0,
											BON_CTRL_STRING_VLQ, STR_VLQ_1.size(), STR_VLQ_1, 0,
											
											BON_SHORT_STRING(1), 'b', 0,
											BON_CTRL_STRING_VLQ,
											0x80 | STR_VLQ_2.size()/128, STR_VLQ_2.size()-128,
											STR_VLQ_2, 0,
											
											BON_CTRL_OBJ_END
											);
			  
			  require_identical(data, size, expected);
		  },
		  
		  
		  [=](bon_r_doc* bon) {
			   REQUIRE( bon->error == BON_SUCCESS );
			  
			  auto root = bon_r_get_block(bon, 0);
			  REQUIRE( root );
			  
			  auto read_key = [&](string key) {
				  CAPTURE( key );
				  auto val = bon_r_get_key(root, key.c_str());
				  REQUIRE( val );
				  return val;
			  };
			  
			  auto test_key_val = [&](string key, string val_str) {
				  CAPTURE( key );
				  CAPTURE( val_str );

				  auto val = read_key(key);
				  REQUIRE( val->type == BON_VALUE_STRING );
				  REQUIRE( val->u.str.size == val_str.size() );
				  REQUIRE( (const char*)val->u.str.ptr == val_str );
			  };
			  
			  auto test_key_int = [&](string key, int64_t val) {
				  CAPTURE( key );
				  CAPTURE( val );
				  
				  auto v = read_key(key);
				  
				  if (val >= 0) {
					  REQUIRE( v->type == BON_VALUE_UINT64 );
					  REQUIRE( v->u.u64 == (uint64_t)val );
				  } else {
					  REQUIRE( v->type == BON_VALUE_SINT64 );
					  REQUIRE( v->u.s64 == val );
				  }
			  };
			  
			  
			  REQUIRE ( read_key("nil")->type == BON_VALUE_NIL );
			  
			  auto true_val = read_key("true");
			  REQUIRE ( true_val->type == BON_VALUE_BOOL );
			  REQUIRE ( true_val->u.boolean == BON_TRUE  );
			  
			  test_key_int("sevent",   7);
			  test_key_int("neg_1",   -1);
			  test_key_int("42",      42);
			  test_key_int("-0x80",   -0x80);
			  test_key_int("0x7fff",  0x7fff);
			  
			  test_key_val("a", STR_VLQ_1);
			  test_key_val("b", STR_VLQ_2);
		  }
	);
}
