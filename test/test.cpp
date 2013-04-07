//
//  main.cpp
//  test
//
//  Created by emilk on 2013-04-05.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//


#define CATCH_CONFIG_MAIN
#include "catch.hpp"


extern "C" {
#include "bon.h"
}

// File size (stat):
#include <sys/stat.h>
#include <sys/types.h>

#include <functional>


using namespace std;


// Checks a binary byte stream against code supplied values
class Verifier
{
public:
	Verifier(const uint8_t* ptr, bon_size size) : m_ptr(ptr), m_size(size) { }
	
	bon_size size() const { return m_size; }
	bool eof() const { return m_ix == m_size; }
	
	void parse_n(const uint8_t* data, bon_size n) {
		REQUIRE( (m_ix+n) <= m_size );
		
		for (bon_size i=0; i<n; ++i) {
			SCOPED_CAPTURE(" i: " << i);
			REQUIRE( m_ptr[m_ix + i] == data[i] );
		}
		m_ix += n;
	}
	
	void parse_n(const void* data, bon_size n) {
		return parse_n((const uint8_t*)data, n);
	}
	
	void parse(uint8_t v) {
		REQUIRE(m_ix < m_size);
		SCOPED_CAPTURE("m_ix: " << m_ix);
		REQUIRE(m_ptr[m_ix] == v);
		++m_ix;
	}
	
	void parse(int8_t v)         { parse((uint8_t)v); }
	void parse(char v)           { parse((uint8_t)v); }
	void parse(bon_ctrl v)       { parse((uint8_t)v); }
	void parse(const string& v)  { parse_n(v.c_str(), v.size()); }
	
	void parse(unsigned long v) {
		assert(v < 256);
		parse( (uint8_t)v );
	}
	
	void parse(unsigned v) {
		parse((unsigned long)v);
	}
	
	void parse(long v) {
		if (v < 0) {
			assert(-128 <= v);
			parse( (int8_t)v );
		} else {
			assert(0 <= v);
			assert(v < 256);
			parse( (uint8_t)v );
		}
	}
	
	void parse(int v) {
		parse( (long)v );
	}
	
	void parse(float v) {
		parse_n(&v, sizeof(v));
	}
	
	void parse(double v) {
		parse_n(&v, sizeof(v));
	}
	
	
	void operator()() { }
	
	template<typename Head, typename... Tail>
	void operator()(const Head& head, Tail... tail)
	{
		parse(head);
		operator()(tail...);
	}
	
private:
	const uint8_t*  m_ptr;
	bon_size        m_size;
	bon_size        m_ix = 0;
};




bon_value* read_key(bon_r_doc* B, bon_value* root, string key) {
	CAPTURE( key );
	auto val = bon_r_get_key(B, root, key.c_str());
	REQUIRE( val );
	return val;
};


void test_val_int(bon_r_doc* B, bon_value* bv, int64_t iv)
{
	REQUIRE( bv );
	REQUIRE( bon_r_is_int(B, bv) );
	REQUIRE( bon_r_int(B, bv) == iv);
}

void test_val_float(bon_r_doc* B, bon_value* bv, float fv)
{
	REQUIRE( bv );
	REQUIRE( bon_r_is_double(B, bv) );
	REQUIRE( bon_r_float(B, bv) == fv);
}


void test_key_int(bon_r_doc* B, bon_value* root, string key, int64_t iv) {
	CAPTURE( key );
	CAPTURE( iv );
	
	auto bv = read_key(B, root, key);
	test_val_int(B, bv, iv);
};


typedef std::function<void(bon_w_doc*)>  Writer;
typedef std::function<void(Verifier&)>     Verifyer;
typedef std::function<void(bon_r_doc*)>  Reader;


void test(Writer w, Verifyer v, Reader r)
{
	bon_byte_vec vec = {0,0,0};
	
	//SECTION( "build", "writing bon" )
	{
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_FLAG_DEFAULT);
		w(B);
		bon_w_close_doc( B );
	}
	
	if (v)
	{
		//SECTION( "test", "checking the binary data" )
		{
			Verifier p(vec.data, vec.size);
			v(p);
			REQUIRE(p.eof());
		}
	}
	
	if (r)
	{
		//SECTION( "read", "parsing the bon file" )
		{
			bon_r_doc* B = bon_r_open(vec.data, vec.size);
			r(B);
			bon_r_close(B);
		}
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



TEST_CASE( "BON/basic types", "Writes, verifies and reads all the basic types of BON" )
{
	
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
		  [=](bon_w_doc* B) {
			  bon_w_header(B);
			  
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "nil",     BON_ZERO_ENDED);   bon_w_nil(B);
			  bon_w_key(B, "true",    BON_ZERO_ENDED);   bon_w_bool(B, BON_TRUE);
			  bon_w_key(B, "seven",   BON_ZERO_ENDED);   bon_w_sint64(B, 7);
			  bon_w_key(B, "neg_1",   BON_ZERO_ENDED);   bon_w_sint64(B, -1);
			  bon_w_key(B, "42",      BON_ZERO_ENDED);   bon_w_sint64(B, 42);
			  bon_w_key(B, "-0x80",   BON_ZERO_ENDED);   bon_w_sint64(B, -0x80);
			  bon_w_key(B, "0x7fff",  BON_ZERO_ENDED);   bon_w_sint64(B, 0x7fff);
			  
			  bon_w_key(B, "f_pi", BON_ZERO_ENDED);  bon_w_float(B, 3.14f);
			  bon_w_key(B, "d_pi", BON_ZERO_ENDED);  bon_w_double(B, 3.14);
			  		  
			  
			  bon_w_key(B, "a", BON_ZERO_ENDED);
			  bon_w_string(B, STR_VLQ_1.c_str(), BON_ZERO_ENDED);
			  
			  bon_w_key(B, "b", BON_ZERO_ENDED);
			  bon_w_string(B, STR_VLQ_2.c_str(), BON_ZERO_ENDED);
			  
			  bon_w_end_obj(B);
			  
			  REQUIRE( bon_w_error(B) == BON_SUCCESS );
		  },
		  
		  
		  [=](Verifier& p) {
			  p( "BON0" );
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(3), "nil",     0,  BON_CTRL_NIL                 );
			  p( BON_SHORT_STRING(4), "true",    0,  BON_CTRL_TRUE                );
			  p( BON_SHORT_STRING(5), "seven",   0,  7                            );
			  p( BON_SHORT_STRING(5), "neg_1",   0,  0xff                         );
			  p( BON_SHORT_STRING(2), "42",      0,  BON_CTRL_UINT8, 42           );
			  p( BON_SHORT_STRING(5), "-0x80",   0,  BON_CTRL_SINT8, -0x80        );
			  p( BON_SHORT_STRING(6), "0x7fff",  0,  BON_CTRL_UINT16, 0xff, 0x7f  );
			  
			  p( BON_SHORT_STRING(4), "f_pi",  0,  BON_CTRL_FLOAT32, 3.14f  );
			  p( BON_SHORT_STRING(4), "d_pi",  0,  BON_CTRL_FLOAT64, 3.14   );
			  
			  p( BON_SHORT_STRING(1), 'a', 0                          );
			  p( BON_CTRL_STRING_VLQ, STR_VLQ_1.size(), STR_VLQ_1, 0  );
			  
			  p( BON_SHORT_STRING(1), 'b', 0                        );
			  p( BON_CTRL_STRING_VLQ                                );
			  p( 0x80 | STR_VLQ_2.size()/128, STR_VLQ_2.size() & 0x7f  );
			  p( STR_VLQ_2, 0                                       );
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  auto test_key_val = [&](string key, string val_str) {
				  CAPTURE( key );
				  CAPTURE( val_str );

				  auto val = read_key(B, root, key);
				  REQUIRE( bon_r_is_string(B, val) );
				  REQUIRE( bon_r_strlen(B, val) == val_str.size() );
				  REQUIRE( bon_r_cstr(B, val) == val_str );
			  };
			  
			  auto test_key_float = [&](string key, double val) {
				  CAPTURE( key );
				  CAPTURE( val );
				  
				  auto v = read_key(B, root, key);
				  
				  REQUIRE( bon_r_is_double(B, v) );
				  REQUIRE( bon_r_double(B, v) == val );
			  };
			  
			  
			  REQUIRE ( bon_r_is_nil(B, read_key(B, root, "nil")) );
			  
			  auto true_val = read_key(B, root, "true");
			  REQUIRE ( bon_r_is_bool(B, true_val) );
			  REQUIRE ( bon_r_bool(B, true_val) == BON_TRUE );
			  
			  test_key_int(B, root, "seven",   7);
			  test_key_int(B, root, "neg_1",   -1);
			  test_key_int(B, root, "42",      42);
			  test_key_int(B, root, "-0x80",   -0x80);
			  test_key_int(B, root, "0x7fff",  0x7fff);
			  
			  test_key_float("f_pi", 3.14f);
			  test_key_float("d_pi", 3.14);
			  
			  test_key_val("a", STR_VLQ_1);
			  test_key_val("b", STR_VLQ_2);
		  }
	);
}

TEST_CASE( "BON/lists & objects", "Tests nested lists and objects" )
{	
	/*
	 {"lists":[1,[2,3]]}	 
	 */
	
	test(
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "lists", BON_ZERO_ENDED);
			  bon_w_begin_list(B);
			  bon_w_uint64(B, 1);
			  bon_w_begin_list(B);
			  bon_w_uint64(B, 2);
			  bon_w_uint64(B, 3);
			  bon_w_end_list(B);
			  bon_w_end_list(B);
			  
			  bon_w_end_obj(B);
			  
			  REQUIRE( bon_w_error(B) == BON_SUCCESS );
		  },
		  
		  [=](Verifier& p) {
			  REQUIRE( p.size() == (size_t)16 );
			  
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(5), "lists", 0 );
			  
			  p( BON_CTRL_LIST_BEGIN                           );
			  p( 1                                             );
			  p( BON_CTRL_LIST_BEGIN, 2, 3, BON_CTRL_LIST_END  );
			  p( BON_CTRL_LIST_END                             );
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  auto lists = read_key(B,  root, "lists" );
			  REQUIRE( bon_r_is_list(B, lists) );
			  REQUIRE( bon_r_list_size(B, lists) == (bon_size)2 );
			  test_val_int( B, bon_r_list_elem(B, lists, 0), 1 );
			  
			  auto inner = bon_r_list_elem(B, lists, 1);
			  REQUIRE( bon_r_is_list(B, inner) );
			  REQUIRE( bon_r_list_size(B, inner) == (bon_size)2 );
			  test_val_int( B, bon_r_list_elem(B, inner, 0), 2 );
			  test_val_int( B, bon_r_list_elem(B, inner, 1), 3 );
		  }
		);
}


TEST_CASE( "BON/blocks", "Blocks and references" )
{
	/*
	 create root object for placing in lagter
	 */
	bon_byte_vec block_1_vec = {0,0,0};
	bon_w_doc* block_1 = bon_w_new_doc(bon_vec_writer, &block_1_vec, BON_FLAG_DEFAULT);
	bon_w_begin_obj(block_1);
	bon_w_key(block_1, "ref2", BON_ZERO_ENDED);  bon_w_block_ref(block_1, 2);
	bon_w_key(block_1, "ref1337", BON_ZERO_ENDED);  bon_w_block_ref(block_1, 1337);
	bon_w_end_obj(block_1);
	REQUIRE( bon_w_error(block_1) == 0 );
	bon_w_close_doc(block_1);
	
	
	
	test(
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_block(B, 1, block_1_vec.data, block_1_vec.size);
			  
			  bon_w_begin_block(B, 2);
			  bon_w_uint64(B, 12);
			  bon_w_end_block(B);
			  
			  bon_w_begin_block(B, 1337);
			  bon_w_uint64(B, 13);
			  bon_w_end_block(B);
			  
			  bon_w_begin_block(B, 0);
			  bon_w_block_ref(B, 1);
			  bon_w_end_block(B);
			  
			  REQUIRE( bon_w_error(B) == BON_SUCCESS );
		  },
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_BLOCK_BEGIN, 1, (int)block_1_vec.size);
			  p( BON_CTRL_OBJ_BEGIN );
			  p( BON_SHORT_STRING(4), "ref2",    0,  BON_SHORT_BLOCK(2) );
			  p( BON_SHORT_STRING(7), "ref1337", 0,  BON_CTRL_BLOCK_REF, 0x80 + 1337/128, 1337 & 0x7f );
			  p( BON_CTRL_OBJ_END );
			  p( BON_CTRL_BLOCK_END);
			  
			  p( BON_CTRL_BLOCK_BEGIN, 2, 0,                             12, BON_CTRL_BLOCK_END);
			  p( BON_CTRL_BLOCK_BEGIN, 0x80 + 1337/128, 1337 & 0x7f, 0,  13, BON_CTRL_BLOCK_END);
			  p( BON_CTRL_BLOCK_BEGIN, 0, 0,  BON_SHORT_BLOCK(1), BON_CTRL_BLOCK_END);
		  },
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  test_key_int(B, root, "ref2", 12);
			  test_key_int(B, root, "ref1337", 13);
		  }
	);
}


TEST_CASE( "BON/parse", "Wirting and parsing aggregates" )
{
	const int NVecs = 2;
	
	struct InVert {
		double   pos[3];
		float    normal[3];
		uint8_t  color[4];
	};
	
	static_assert(sizeof(InVert) == 3*sizeof(InVert::pos[0]) + 3*sizeof(InVert::normal[0]) + 4, "pack");
	
	InVert inVecs[NVecs] = {
		{
			{1,2,3},
			{0,0,1},
			{255,196,128,64}
		},
		{
			{4,5,6},
			{1,0,0},
			{64,128,196,255}
		}
	};
	
	test(
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "vecs", BON_ZERO_ENDED);
			  bon_w_aggr_fmt(B, inVecs, sizeof(inVecs),
								  "[#{[3d][3f][4u8]}]", (bon_size)NVecs, "pos", "normal", "color");
			  
			  bon_w_end_obj(B);
			  
			  REQUIRE( bon_w_error(B) == BON_SUCCESS );
		  },
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(4), "vecs", 0  );
			  p( BON_SHORT_ARRAY(2)              );
			  
			  p( BON_SHORT_STRUCT(3) );
			  p( BON_SHORT_STRING(3), "pos", 0);
			  p( BON_SHORT_ARRAY(3), BON_CTRL_FLOAT64 );
			  p( BON_SHORT_STRING(6), "normal", 0);
			  p( BON_SHORT_ARRAY(3), BON_CTRL_FLOAT32 );
			  p( BON_SHORT_STRING(5), "color", 0);
			  p( BON_SHORT_BYTE_ARRAY(4) );
			  
			  p( 1.0,  2.0,  3.0  );
			  p( 0.0f, 0.0f, 1.0f );
			  p( 255,196,128,64 );
			  
			  p( 4.0 , 5.0 , 6.0  );
			  p( 1.0f, 0.0f, 0.0f );
			  p( 64,128,196,255 );
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  // ------------------------------------------------
			  
			  auto vecs  = read_key(B, root, "vecs");
			  REQUIRE( bon_r_is_list(B, vecs) );
			  REQUIRE( bon_r_list_size(B, vecs) == (bon_size)NVecs );
			  
			  // ------------------------------------------------
			  // Read using aggregate API:
			  
			  {
				  auto vertPtr = (const InVert*)bon_r_aggr_ptr_fmt(B, vecs, 2*sizeof(InVert),
																					"[#{[3d][3f][4u8]}]", (bon_size)NVecs,
																					"pos", "normal", "color");
				  REQUIRE( vertPtr );
				  REQUIRE( vertPtr[1].color[0] == 64 );
			  }
			  
			  {
				  struct alignas(1) OutVert {
					  uint8_t   color[4];
					  float     pos[3];
				  };
				  
				  static_assert(sizeof(OutVert) == 4 + 3 * sizeof(OutVert::pos[0]), "pack");
				  
				  OutVert outVerts[NVecs];
				  
				  bool win = bon_r_aggr_read_fmt(B, vecs, outVerts, sizeof(outVerts),
															"[2{[4u8][3f]}]", "color", "pos");
				  REQUIRE( win );
				  REQUIRE( outVerts[0].color[0]  == 255  );
				  REQUIRE( outVerts[0].pos[0]    == 1.0f );
				  REQUIRE( outVerts[1].pos[2]    == 6.0f );
			  }
			  
			  // ------------------------------------------------
			  // Read using tradional API (will convert aggregate):
			  
			  auto v1    = bon_r_list_elem(B, vecs, 1);
			  
			  auto pos   = read_key(B, v1, "pos");
			  REQUIRE( bon_r_is_list(B, pos) );
			  REQUIRE( bon_r_list_size(B, pos) == (bon_size)3 );
			  test_val_float(B, bon_r_list_elem(B, pos, 0), 4.0f);
			  test_val_float(B, bon_r_list_elem(B, pos, 1), 5.0f);
			  test_val_float(B, bon_r_list_elem(B, pos, 2), 6.0f);
			  
			  auto color   = read_key(B, v1, "color");
			  REQUIRE( bon_r_is_list(B, color) );
			  REQUIRE( bon_r_list_size(B, color) == (bon_size)4 );
			  test_val_int(B, bon_r_list_elem(B, color, 0), 64);
			  test_val_int(B, bon_r_list_elem(B, color, 1), 128);
			  test_val_int(B, bon_r_list_elem(B, color, 2), 196);
			  test_val_int(B, bon_r_list_elem(B, color, 3), 255);
		  }
		  );
}
