//
//  main.cpp
//  test
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).


#define CATCH_CONFIG_MAIN
#include "catch.hpp"

extern "C" {
#include <bon/bon.h>
#include <bon/private.h>
#include <bon/crc32.h>
}

#include <functional>


using namespace std;


TEST_CASE( "crc32", "Santiy checking of crc32 implementation" )
{
	uint8_t data[] = { 'a', 'b', 'c', 'd' };
	const uint32_t crc = crc_calc(data, sizeof(data));
	REQUIRE( crc == 0xED82CD11 );
}


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
			SCOPED_INFO(" i: " << i);
			SCOPED_INFO("in: " << std::hex << (unsigned)m_ptr[m_ix + i]);
			SCOPED_INFO("v:  " << std::hex << (unsigned)data[i]);
			REQUIRE( m_ptr[m_ix + i] == data[i] );
		}
		m_ix += n;
	}
	
	void parse_n(const void* data, bon_size n) {
		return parse_n((const uint8_t*)data, n);
	}
	
	void parse(uint8_t v) {
		REQUIRE(m_ix < m_size);
		SCOPED_INFO("m_ix: " << m_ix);
		SCOPED_INFO("in: " << std::hex << (unsigned)m_ptr[m_ix]);
		SCOPED_INFO("v:  " << std::hex << (unsigned)v);
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

void writeData(bon_byte_vec* vec, std::string bon_name)
{
	FILE* fp = fopen(bon_name.c_str(), "wb");
	assert( fp );
	fwrite(vec->data, 1, vec->size, fp);
	fclose(fp);
}

void test(std::string baseName, Writer w, Verifyer v, Reader r)
{
	bon_byte_vec vec = {0,0,0};
	
	//SECTION( "build", "writing bon" )
	{
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAG_DEFAULT);
		w(B);
		REQUIRE( bon_w_close_doc(B) == BON_SUCCESS );
	}
	
	if (baseName != "") {
		writeData(&vec, baseName + ".bon");
	}
	
	if (v)
	{
		//SECTION( "test", "checking the binary data" )
		{
			Verifier p(vec.data, vec.size);
			p( "BON0" );
			v(p);
			p( BON_CTRL_FOOTER );
			REQUIRE(p.eof());
		}
	}
	
	if (r)
	{
		//SECTION( "read", "parsing the bon file" )
		{
			bon_r_doc* B = bon_r_open(vec.data, vec.size, BON_R_FLAG_DEFAULT);
			r(B);
			REQUIRE( bon_r_error(B) == BON_SUCCESS );
			bon_r_close(B);
		}
	}
	
	free(vec.data);
}

TEST_CASE( "BON/code ranges", "Code ranges" ) {
	REQUIRE((BON_SHORT_NEG_INT_START + 16)  == 256);
	
	printf("sizeof(bon_value): %d\n", (int)sizeof(bon_value));
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
		BON_CTRL_FOOTER,         BON_CTRL_FOOTER_CRC,
		BON_CTRL_NIL,            
		BON_CTRL_SINT8,          
		BON_CTRL_UINT8,          
		BON_CTRL_SINT16_LE,      BON_CTRL_SINT16_BE,
		BON_CTRL_UINT16_LE,      BON_CTRL_UINT16_BE,
		BON_CTRL_SINT32_LE,      BON_CTRL_SINT32_BE,
		BON_CTRL_UINT32_LE,      BON_CTRL_UINT32_BE,
		BON_CTRL_SINT64_LE,      BON_CTRL_SINT64_BE,
		BON_CTRL_UINT64_LE,      BON_CTRL_UINT64_BE,
		BON_CTRL_FLOAT_LE,     BON_CTRL_FLOAT_BE,
		BON_CTRL_DOUBLE_LE,     BON_CTRL_DOUBLE_BE,
		BON_CTRL_LIST_BEGIN,     BON_CTRL_LIST_END,    // 0x5B   0x5D
		BON_CTRL_OBJ_BEGIN,      BON_CTRL_OBJ_END,     // 0x7B   0x7D
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
	
	test("basic_types",
		  
		  // Write 
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "nil");      bon_w_nil(B);
			  bon_w_key(B, "true");     bon_w_bool(B, BON_TRUE);
			  bon_w_key(B, "seven");    bon_w_sint64(B, 7);
			  bon_w_key(B, "neg_1");    bon_w_sint64(B, -1);
			  bon_w_key(B, "42");       bon_w_sint64(B, 42);
			  bon_w_key(B, "-0x80");    bon_w_sint64(B, -0x80);
			  bon_w_key(B, "0x7fff");   bon_w_sint64(B, 0x7fff);
			  
			  bon_w_key(B, "f_pi");  bon_w_float(B, 3.14f);
			  bon_w_key(B, "d_pi");  bon_w_double(B, 3.14);
			  		  
			  
			  bon_w_key(B, "a");
			  bon_w_cstring(B, STR_VLQ_1.c_str());
			  
			  bon_w_key(B, "b");
			  bon_w_cstring(B, STR_VLQ_2.c_str());
			  
			  bon_w_end_obj(B);
		  },
		  
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(3), "nil",     0,  BON_CTRL_NIL                 );
			  p( BON_SHORT_STRING(4), "true",    0,  BON_CTRL_TRUE                );
			  p( BON_SHORT_STRING(5), "seven",   0,  7                            );
			  p( BON_SHORT_STRING(5), "neg_1",   0,  0xff                         );
			  p( BON_SHORT_STRING(2), "42",      0,  BON_CTRL_UINT8, 42           );
			  p( BON_SHORT_STRING(5), "-0x80",   0,  BON_CTRL_SINT8, -0x80        );
			  p( BON_SHORT_STRING(6), "0x7fff",  0,  BON_CTRL_UINT16, 0xff, 0x7f  );
			  
			  p( BON_SHORT_STRING(4), "f_pi",  0,  BON_CTRL_FLOAT, 3.14f  );
			  p( BON_SHORT_STRING(4), "d_pi",  0,  BON_CTRL_DOUBLE, 3.14   );
			  
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
	
	test("lists_and_objects",
		  
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "lists");
			  bon_w_begin_list(B);
			  bon_w_uint64(B, 1);
			  bon_w_begin_list(B);
			  bon_w_uint64(B, 2);
			  bon_w_uint64(B, 3);
			  bon_w_end_list(B);
			  bon_w_end_list(B);
			  
			  bon_w_end_obj(B);
		  },
		  
		  [=](Verifier& p) {
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
	bon_w_doc* block_1 = bon_w_new_doc(bon_vec_writer, &block_1_vec, BON_W_FLAG_SKIP_HEADER_FOOTER);
	bon_w_begin_obj(block_1);
	bon_w_key(block_1, "ref2");  bon_w_block_ref(block_1, 2);
	bon_w_key(block_1, "ref1337");  bon_w_block_ref(block_1, 1337);
	bon_w_end_obj(block_1);
	REQUIRE( bon_w_close_doc(block_1) == BON_SUCCESS );
	
	
	
	test("blocks",
		  
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


TEST_CASE( "BON/parse", "Writing and parsing aggregates" )
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
	
	test("parse",
		  
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "vecs");
			  bon_w_pack_fmt(B, inVecs, sizeof(inVecs),
								  "[#{$[3d]$[3f]$[4u8]}]", NVecs, "pos", "normal", "color");
			  
			  bon_w_end_obj(B);
		  },
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(4), "vecs", 0  );
			  p( BON_SHORT_ARRAY(2)              );
			  
			  p( BON_SHORT_STRUCT(3) );
			  p( BON_SHORT_STRING(3), "pos", 0);
			  p( BON_SHORT_ARRAY(3), BON_CTRL_DOUBLE );
			  p( BON_SHORT_STRING(6), "normal", 0);
			  p( BON_SHORT_ARRAY(3), BON_CTRL_FLOAT );
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
				  auto vertPtr = (const InVert*)bon_r_unpack_ptr_fmt(B, vecs, 2*sizeof(InVert),
																					"[#{$[3d]$[3f]$[4u8]}]",
																				   NVecs, "pos", "normal", "color");
				  REQUIRE( vertPtr );
				  REQUIRE( vertPtr[1].color[0] == 64 );
			  }
			  
			  {
				  struct OutVert {
					  uint8_t   color[4];
					  float     pos[3];
				  };
				  
				  static_assert(sizeof(OutVert) == 4 + 3 * sizeof(OutVert::pos[0]), "pack");
				  
				  OutVert outVerts[NVecs];
				  
				  bool win = bon_r_unpack_fmt(B, vecs, outVerts, sizeof(outVerts),
															"[2{$[4u8]$[3f]}]", "color", "pos");
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




TEST_CASE( "BON/unpack/numeric conversions of packed data", "Automatic numeric conversions when unpacking BON data" )
{	
	test("unpack_numeric",
		  
		  // Write
		  [=](bon_w_doc* B) {
			  int8_t    s8[]   =  { -8   };
			  uint8_t   u8[]   =  { +8   };
			  int16_t   s16[]  =  { -16  };
			  uint16_t  u16[]  =  { +16  };
			  //int32_t   s32[]  =  { -32  };
			  //uint32_t  u32[]  =  { +32  };
			  int64_t   s64[]  =  { -64  };
			  uint64_t  u64[]  =  { +64  };
			  float     fn[]   =  { -3.14f    };
			  float     fp[]   =  { +3.14f    };
			  double    dn[]   =  { -2.71828  };
			  double    dp[]   =  { +2.71828  };
			  
			  bon_w_begin_obj( B );
			  
			  bon_w_key(B, "s8");   bon_w_pack_array(B,  s8, sizeof(s8),  1, BON_TYPE_SINT8   );
			  bon_w_key(B, "u8");   bon_w_pack_array(B,  u8, sizeof(u8),  1, BON_TYPE_UINT8   );
			  bon_w_key(B, "s16");  bon_w_pack_array(B, s16, sizeof(s16), 1, BON_TYPE_SINT16  );
			  bon_w_key(B, "u16");  bon_w_pack_array(B, u16, sizeof(u16), 1, BON_TYPE_UINT16  );
			  bon_w_key(B, "s64");  bon_w_pack_array(B, s64, sizeof(s64), 1, BON_TYPE_SINT64  );
			  bon_w_key(B, "u64");  bon_w_pack_array(B, u64, sizeof(u64), 1, BON_TYPE_UINT64  );
			  bon_w_key(B, "fn");   bon_w_pack_array(B,  fn, sizeof(fn),  1, BON_TYPE_FLOAT );
			  bon_w_key(B, "fp");   bon_w_pack_array(B,  fp, sizeof(fp),  1, BON_TYPE_FLOAT );
			  bon_w_key(B, "dn");   bon_w_pack_array(B,  dn, sizeof(dn),  1, BON_TYPE_DOUBLE );
			  bon_w_key(B, "dp");   bon_w_pack_array(B,  dp, sizeof(dp),  1, BON_TYPE_DOUBLE );
			  
			  bon_w_end_obj( B );
		  },
		  
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(2), "s8",  0,  BON_SHORT_ARRAY(1), BON_CTRL_SINT8,    -8         );
			  p( BON_SHORT_STRING(2), "u8",  0,  BON_SHORT_BYTE_ARRAY(1),               +8         );
			  p( BON_SHORT_STRING(3), "s16", 0,  BON_SHORT_ARRAY(1), BON_CTRL_SINT16,  -16,  0xff  );
			  p( BON_SHORT_STRING(3), "u16", 0,  BON_SHORT_ARRAY(1), BON_CTRL_UINT16,  +16,  0     );
			  p( BON_SHORT_STRING(3), "s64", 0,  BON_SHORT_ARRAY(1), BON_CTRL_SINT64,  -64,  0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff  );
			  p( BON_SHORT_STRING(3), "u64", 0,  BON_SHORT_ARRAY(1), BON_CTRL_UINT64,  +64,  0,    0,    0,     0,    0,    0,    0     );
			  
			  p( BON_SHORT_STRING(2), "fn",  0,  BON_SHORT_ARRAY(1), BON_CTRL_FLOAT,  -3.14f    );
			  p( BON_SHORT_STRING(2), "fp",  0,  BON_SHORT_ARRAY(1), BON_CTRL_FLOAT,  +3.14f    );
			  p( BON_SHORT_STRING(2), "dn",  0,  BON_SHORT_ARRAY(1), BON_CTRL_DOUBLE,  -2.71828  );
			  p( BON_SHORT_STRING(2), "dp",  0,  BON_SHORT_ARRAY(1), BON_CTRL_DOUBLE,  +2.71828  );
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  int8_t    s8[]   =  { 0 };
			  uint64_t  u64[]  =  { 0 };
			  float     f[]    =  { 0 };
			  double    d[]    =  { 0 };
			  
			  //------------------------------------------------------------------------------
			  // Converting to s8
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s8"),  s8, sizeof(s8), "[1i8]") );
			  REQUIRE( *s8 == -8 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u16"), s8, sizeof(s8), "[1i8]") );
			  REQUIRE( *s8 == +16 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s64"), s8, sizeof(s8), "[1i8]") );
			  REQUIRE( *s8 == -64 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "fn"),  s8, sizeof(s8), "[1i8]") );
			  REQUIRE( *s8 == -3 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "dp"),  s8, sizeof(s8), "[1i8]") );
			  REQUIRE( *s8 == +2 );
			  
			  //------------------------------------------------------------------------------
			  // Converting to u64
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u8"),  u64, sizeof(u64), "[1u64]") );
			  REQUIRE( (int)*u64 == +8 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u16"), u64, sizeof(u64), "[1u64]") );
			  REQUIRE( (int)*u64 == +16 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u64"), u64, sizeof(u64), "[1u64]") );
			  REQUIRE( (int)*u64 == +64 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "fp"),  u64, sizeof(u64), "[1u64]") );
			  REQUIRE( (int)*u64 == +3 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "dp"),  u64, sizeof(u64), "[1u64]") );
			  REQUIRE( (int)*u64 == +2 );
			  
			  //------------------------------------------------------------------------------
			  // Converting to float 32
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s8"),  f, sizeof(f), "[1f]") );
			  REQUIRE( *f == -8 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u16"), f, sizeof(f), "[1f]") );
			  REQUIRE( *f == +16 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s64"), f, sizeof(f), "[1f]") );
			  REQUIRE( *f == -64 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "fp"),  f, sizeof(f), "[1f]") );
			  REQUIRE( *f == +3.14f );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "dn"),  f, sizeof(f), "[1f]") );
			  REQUIRE( *f == -2.71828f );
			  
			  //------------------------------------------------------------------------------
			  // Converting to double 64
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s8"),  d, sizeof(d), "[1d]") );
			  REQUIRE( *d == -8 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "u16"), d, sizeof(d), "[1d]") );
			  REQUIRE( *d == +16 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "s64"), d, sizeof(d), "[1d]") );
			  REQUIRE( *d == -64 );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "fp"),  d, sizeof(d), "[1d]") );
			  REQUIRE( *d == +3.14f );
			  REQUIRE( bon_r_unpack_fmt(B, read_key(B, root, "dn"),  d, sizeof(d), "[1d]") );
			  REQUIRE( *d == -2.71828 );
		  }
		  );
}



TEST_CASE( "BON/unpack/conversions", "Automatic conversions when unpacking BON data" )
{
	test("unpack_convert",
		  
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "-8");  bon_w_sint64(B, -8);
			  bon_w_key(B, "0x12345678");  bon_w_sint64(B, 0x12345678);
			  
			  bon_w_end_obj(B);
		  },
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(2),  "-8",         0,   -8 );
			  p( BON_SHORT_STRING(10), "0x12345678", 0,    BON_CTRL_UINT32, 0x78, 0x56, 0x34, 0x12 );
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  struct f_i32_t { float f; int32_t i32; };
			  f_i32_t f_i32 = {0,0};
			  REQUIRE( bon_r_unpack_fmt(B, root,  &f_i32, sizeof(f_i32), "{$f$i32}", "-8", "0x12345678") );
			  REQUIRE( f_i32.f == -8 );
			  REQUIRE( f_i32.i32 == 0x12345678 );
			  
			  struct i64_d_t { int64_t i64; double d; };
			  i64_d_t i64_d = {0,0};
			  REQUIRE( bon_r_unpack_fmt(B, root,  &i64_d, sizeof(i64_d), "{$i64$d}", "-8", "0x12345678") );
			  REQUIRE( i64_d.i64  == -8 );
			  REQUIRE( i64_d.d    == 0x12345678 );
		  }
		  );
}



TEST_CASE( "BON/integers", "Writing and reading of integers" )
{
	std::vector<int64_t> ints = {
		0, 1, 14, 15, 16, 17, 127, 128, 129, 255, 256, 257,
		(1<<15)-1, (1<<15), (1<<15)+1,
		(1<<16)-1, (1<<16), (1<<16)+1,
		(1ULL<<32)-1, (1ULL<<32), (1ULL<<32)+1,
		(1ULL<<48)-1
	};
	
	{
		// Negatives too:
		auto n = ints.size();
		for (size_t i=0; i<n; ++i) {
			ints.push_back( - ints[i] );
		}
	}
	
	
	test("integers",
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "ints");
			  bon_w_begin_list(B);
			  for (auto i : ints) {
				  bon_w_sint64(B, i);
			  }
			  bon_w_end_list(B);
			  
			  bon_w_end_obj(B);
		  },
		  
		  nullptr, // Skip verify
		  
		  [=](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  auto list = read_key(B, root, "ints");
			  
			  REQUIRE( bon_r_list_size(B, list) == ints.size() );
			  
			  for (size_t i=0; i<ints.size(); ++i) {
				  auto elem = bon_r_list_elem(B, list, i);
				  REQUIRE( ints[i] == bon_r_int(B, elem) );
			  }
		  }
		  );
}


TEST_CASE( "BON/crc/short/pass", "Test of CRC checking" )
{
	bon_byte_vec vec = {0,0,0};
	
	{
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAG_CRC);
		bon_w_begin_obj(B);
		bon_w_end_obj(B);
		REQUIRE( bon_w_close_doc(B) == BON_SUCCESS );
	}
	
	writeData(&vec, "short.bon");
	
	{
		Verifier p(vec.data, vec.size);
		p( "BON0", BON_CTRL_OBJ_BEGIN, BON_CTRL_OBJ_END );
		p( BON_CTRL_FOOTER_CRC, 0x7E, 0xA2, 0x2E, 0x94, BON_CTRL_FOOTER_CRC);
		REQUIRE(p.eof());
	}
	
	{
		bon_r_doc* B = bon_r_open(vec.data, vec.size, BON_R_FLAG_REQUIRE_CRC);
		auto root = bon_r_root(B);
		REQUIRE( root );
		REQUIRE( bon_r_is_object(B, root) );
		REQUIRE( bon_r_obj_size(B, root) == (bon_size)0);
		REQUIRE( bon_r_error(B) == BON_SUCCESS );
		bon_r_close(B);
	}
}


TEST_CASE( "BON/crc/short/fail", "Test of CRC checking" )
{
	bon_byte_vec vec = {0,0,0};
	
	{
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAG_CRC);
		bon_w_begin_obj(B);
		bon_w_end_obj(B);
		REQUIRE( bon_w_close_doc(B) == BON_SUCCESS );
	}
	
	{
		Verifier p(vec.data, vec.size);
		p( "BON0", BON_CTRL_OBJ_BEGIN, BON_CTRL_OBJ_END );
		p( BON_CTRL_FOOTER_CRC, 0x7E, 0xA2, 0x2E, 0x94, BON_CTRL_FOOTER_CRC);
		REQUIRE(p.eof());
		vec.data[5] = 0; // Malicious tampering!
	}
	
	{
		bon_r_doc* B = bon_r_open(vec.data, vec.size, BON_R_FLAG_REQUIRE_CRC);
		REQUIRE( bon_r_error(B) == BON_ERR_BAD_CRC );
		bon_r_close(B);
	}
}


//------------------------------------------------------------------------------



TEST_CASE( "BON/pack/matrix", "Packing a rectangular matirx" )
{
	constexpr size_t W = 4;
	constexpr size_t H = 10;
	float src[W][H];
	
	for (size_t i=0; i<W; ++i) {
		for (size_t j=0; j<H; ++j) {
			src[i][j] = i * 100 + j + 0.1f;
		}
	}
	
	int16_t dst[W][H] = {{0}};
	
	
	test("matrix",
		  
		  // Write
		  [=](bon_w_doc* B) {
			  bon_w_begin_obj(B);
			  
			  bon_w_key(B, "mat");
			  
#if 0
			  bon_w_pack_fmt(B, src, sizeof(src), "[#[#f]]", W, H);
#else
			  bon_w_begin_list(B);
			  for (size_t i=0; i<W; ++i) {
				  bon_w_begin_list(B);
				  for (size_t j=0; j<H; ++j) {
					  bon_w_float(B, src[i][j]);
				  }
				  bon_w_end_list(B);
			  }
			  bon_w_end_list(B);
#endif
			  
			  bon_w_end_obj(B);
		  },
		  
		  [=](Verifier& p) {
			  p( BON_CTRL_OBJ_BEGIN );
			  
			  p( BON_SHORT_STRING(3),  "mat",         0 );
			  
#if 0
			  p( BON_SHORT_ARRAY(W), BON_SHORT_ARRAY(H), BON_CTRL_FLOAT );
			  
			  for (size_t i=0; i<W; ++i) {
				  for (size_t j=0; j<H; ++j) {
					  p( src[i][j] );
				  }
			  }
#else
			  p( BON_CTRL_LIST_BEGIN );
			  for (size_t i=0; i<W; ++i) {
				  p( BON_CTRL_LIST_BEGIN );
				  for (size_t j=0; j<H; ++j) {
					  p( BON_CTRL_FLOAT );
					  p( src[i][j] );
				  }
				  p( BON_CTRL_LIST_END );
			  }
			  p( BON_CTRL_LIST_END );
#endif
			  
			  p( BON_CTRL_OBJ_END );
		  },
		  
		  [&](bon_r_doc* B) {
			  REQUIRE( bon_r_error(B) == BON_SUCCESS );
			  
			  auto root = bon_r_root(B);
			  REQUIRE( root );
			  
			  auto mat = read_key(B, root, "mat");
			  
			  auto win = bon_r_unpack_fmt(B, mat, dst, sizeof(dst), "[#[#i16]]", W, H);
			  REQUIRE( win );
			  
			  for (size_t i=0; i<W; ++i) {
				  for (size_t j=0; j<H; ++j) {
					  REQUIRE( dst[i][j]  ==  (int32_t)src[i][j] );
				  }
			  }
			  
			  REQUIRE( bon_r_list_size(B, mat) == W );
			  REQUIRE( bon_r_list_size(B, bon_r_list_elem(B, mat, 0)) == H );
		  }
	);
}

