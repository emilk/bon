//
//  bon.h
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#ifndef BON_bon_w_h
#define BON_bon_w_h

#include <stdint.h>
#include <stdio.h>

#if defined(__linux__)
#  include <endian.h>
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define __LITTLE_ENDIAN__ 1
#  elif __BYTE_ORDER == __BIG_ENDIAN
#    define __BIG_ENDIAN__ 1
#  endif
#endif

#if !__LITTLE_ENDIAN__ && !__BIG_ENDIAN__
#  error "BON lib doesn't work on mixed endian machines!"
#elif __LITTLE_ENDIAN__ && __BIG_ENDIAN__
#  error "BON lib needs __LITTLE_ENDIAN__ or __BIG_ENDIAN__ defined - not both!"
#endif


//------------------------------------------------------------------------------
// BON common

typedef char bon_bool;

#define BON_TRUE  ((bon_bool)1)
#define BON_FALSE ((bon_bool)0)

typedef size_t bon_size;

#define BON_ZERO_ENDED (bon_size)(-1)  // Valid as argument of string size

typedef uint64_t bon_block_id;
#define BON_BAD_BLOCK_ID (bon_block_id)(-1)

#define BON_INLINE static inline


//------------------------------------------------------------------------------


typedef enum {
	BON_SUCCESS = 0,
	
	// Write errors
	BON_ERR_WRITE_ERROR,        // Writer refused
	BON_ERR_BAD_AGGREGATE_SIZE, // bon_w_pack got data of the wrong size
	
	// Read errors:
	BON_ERR_BAD_CRC,     // Missing or wrong
	BON_ERR_TOO_SHORT,   // Premature end of file
	BON_ERR_BAD_HEADER,  // Missing or corrupt
	BON_ERR_BAD_FOOTER,  // Missing or corrupt
	BON_ERR_BAD_VLQ,
	BON_ERR_MISSING_LIST_END,
	BON_ERR_MISSING_OBJ_END,
	BON_ERR_BAD_CTRL,
	BON_ERR_BAD_KEY,     // Not a string, or string with zeros
	BON_ERR_BAD_AGGREGATE_TYPE,
	BON_ERR_BAD_TYPE,
	BON_ERR_STRING_NOT_ZERO_ENDED,
	BON_ERR_MISSING_TOKEN,
	
	BON_ERR_TRAILING_DATA,  // Data trailing the document
	BON_ERR_BAD_BLOCK,
	BON_ERR_BAD_BLOCK_REF,  // Referring a block with an ID smaller or equal to own
	
	// bw_read_aggregate etc:
	BON_ERR_NARROWING,
	BON_ERR_NULL_OBJ,
	
	BON_NUM_ERR
} bon_error;


// Plain text versions of the above
const char* bon_err_str(bon_error err);


//------------------------------------------------------------------------------


/* The control codes are all in [0x08, 0x10)
 */
typedef enum {
	BON_CTRL_BLOCK_REF   = 0x40,   BON_CTRL_STRING_VLQ  = 0x60,  // @   `
	BON_CTRL_ARRAY_VLQ   = 'A',
	BON_CTRL_HEADER      = 'B',
	BON_CTRL_TUPLE_VLQ   = 'C',    BON_CTRL_STRUCT_VLQ  = 'c',
	BON_CTRL_BLOCK_BEGIN = 'D',    BON_CTRL_BLOCK_END   = 'd',
	BON_CTRL_TRUE        = 'E',    BON_CTRL_FALSE       = 'e',
	BON_CTRL_FOOTER      = 'F',    BON_CTRL_FOOTER_CRC  = 'f',
	BON_CTRL_NIL         = 'N',
	
	BON_CTRL_SINT8       = 'P',
	BON_CTRL_UINT8       = 'Q',
	BON_CTRL_SINT16_LE   = 'R',    BON_CTRL_SINT16_BE   = 'r',
	BON_CTRL_UINT16_LE   = 'S',    BON_CTRL_UINT16_BE   = 's',
	BON_CTRL_SINT32_LE   = 'T',    BON_CTRL_SINT32_BE   = 't',
	BON_CTRL_UINT32_LE   = 'U',    BON_CTRL_UINT32_BE   = 'u',
	BON_CTRL_SINT64_LE   = 'V',    BON_CTRL_SINT64_BE   = 'v',
	BON_CTRL_UINT64_LE   = 'W',    BON_CTRL_UINT64_BE   = 'w',
	
	BON_CTRL_FLOAT_LE  = 'X',    BON_CTRL_FLOAT_BE  = 'x',
	BON_CTRL_DOUBLE_LE  = 'Y',    BON_CTRL_DOUBLE_BE  = 'y',
	
	// Open-ended list and object
	BON_CTRL_LIST_BEGIN  = '[',    BON_CTRL_LIST_END    = ']',  // 0x5B   0x5D
	BON_CTRL_OBJ_BEGIN   = '{',    BON_CTRL_OBJ_END     = '}',  // 0x7B   0x7D
	
#if __LITTLE_ENDIAN__
	BON_CTRL_SINT16   = BON_CTRL_SINT16_LE,
	BON_CTRL_UINT16   = BON_CTRL_UINT16_LE,
	BON_CTRL_SINT32   = BON_CTRL_SINT32_LE,
	BON_CTRL_UINT32   = BON_CTRL_UINT32_LE,
	BON_CTRL_SINT64   = BON_CTRL_SINT64_LE,
	BON_CTRL_UINT64   = BON_CTRL_UINT64_LE,
	BON_CTRL_FLOAT  = BON_CTRL_FLOAT_LE,
	BON_CTRL_DOUBLE  = BON_CTRL_DOUBLE_LE,
#else
	BON_CTRL_SINT16   = BON_CTRL_SINT16_BE,
	BON_CTRL_UINT16   = BON_CTRL_UINT16_BE,
	BON_CTRL_SINT32   = BON_CTRL_SINT32_BE,
	BON_CTRL_UINT32   = BON_CTRL_UINT32_BE,
	BON_CTRL_SINT64   = BON_CTRL_SINT64_BE,
	BON_CTRL_UINT64   = BON_CTRL_UINT64_BE,
	BON_CTRL_FLOAT  = BON_CTRL_FLOAT_BE,
	BON_CTRL_DOUBLE  = BON_CTRL_DOUBLE_BE
#endif
} bon_ctrl;


//------------------------------------------------------------------------------

typedef enum {
	BON_TYPE_BOOL, // target: bon_bool
	BON_TYPE_STRING  = BON_CTRL_STRING_VLQ,  // target: const char*
	
	
	//BON_TYPE_OBJECT  = BON_CTRL_OBJ_BEGIN,
	//BON_TYPE_LIST    = BON_CTRL_LIST_BEGIN,
	
	BON_TYPE_ARRAY   = BON_CTRL_ARRAY_VLQ,
	//BON_TYPE_TUPLE   = BON_CTRL_TUPLE_VLQ,
	BON_TYPE_STRUCT  = BON_CTRL_STRUCT_VLQ,
	
	BON_TYPE_SINT8  = BON_CTRL_SINT8,
	BON_TYPE_UINT8  = BON_CTRL_UINT8,
	
	BON_TYPE_SINT16_LE  = BON_CTRL_SINT16_LE,
	BON_TYPE_SINT16_BE  = BON_CTRL_SINT16_BE,
	BON_TYPE_UINT16_LE  = BON_CTRL_UINT16_LE,
	BON_TYPE_UINT16_BE  = BON_CTRL_UINT16_BE,
	
	BON_TYPE_SINT32_LE  = BON_CTRL_SINT32_LE,
	BON_TYPE_SINT32_BE  = BON_CTRL_SINT32_BE,
	BON_TYPE_UINT32_LE  = BON_CTRL_UINT32_LE,
	BON_TYPE_UINT32_BE  = BON_CTRL_UINT32_BE,
	
	BON_TYPE_SINT64_LE  = BON_CTRL_SINT64_LE,
	BON_TYPE_SINT64_BE  = BON_CTRL_SINT64_BE,
	BON_TYPE_UINT64_LE  = BON_CTRL_UINT64_LE,
	BON_TYPE_UINT64_BE  = BON_CTRL_UINT64_BE,
	
	BON_TYPE_FLOAT_LE = BON_CTRL_FLOAT_LE,
	BON_TYPE_FLOAT_BE = BON_CTRL_FLOAT_BE,
	BON_TYPE_DOUBLE_LE = BON_CTRL_DOUBLE_LE,
	BON_TYPE_DOUBLE_BE = BON_CTRL_DOUBLE_BE,
	
	BON_TYPE_SINT16   = BON_CTRL_SINT16,
	BON_TYPE_UINT16   = BON_CTRL_UINT16,
	BON_TYPE_SINT32   = BON_CTRL_SINT32,
	BON_TYPE_UINT32   = BON_CTRL_UINT32,
	BON_TYPE_SINT64   = BON_CTRL_SINT64,
	BON_TYPE_UINT64   = BON_CTRL_UINT64,
	BON_TYPE_FLOAT  = BON_CTRL_FLOAT,
	BON_TYPE_DOUBLE  = BON_CTRL_DOUBLE
} bon_type_id;

//------------------------------------------------------------------------------

/*
 The general type system.
 This is used for reading and writing.
 For writing, the user may spec up a type and write it.
 For reading, the user may spec up a type and ask BON to read it.
 For reading, the user may inquire BON about the type of the next value.
 */
typedef struct bon_type bon_type;


void      bon_free_type(bon_type* t);

// Functions for creating these types
bon_type* bon_new_type_simple(bon_type_id id);
bon_type* bon_new_type_simple_array(bon_size n, bon_type_id id);
bon_type* bon_new_type_array(bon_size n, bon_type* type);

/*
 struct Vert { float pos[3]; uint8_t color[4]; }
 bon_type* bon_new_type_fmt("{$[3f]$[4u8]}", "pos", "color")
 
 u8 u16 u32 u64   - unsigned int
 i8 i16 i32 i64   - signed integer
 f                - float
 d                - double
 {...}            - object. Interleave $ (key placeholder) and types.
 [3f]             - array of three floats
 [#u8]            - array of bytes (array size given in vargs as bon_size)
   
 NULL on fail
 */
bon_type* bon_new_type_fmt(const char* fmt, ...);

// Returns true if the two types are exactly equal
bon_bool  bon_type_eq(const bon_type* a, const bon_type* b);


//------------------------------------------------------------------------------


typedef struct {
	bon_size  size; // Usage
	bon_size  cap;  // Number of bytes allocated in 'data'
	uint8_t*  data;
} bon_byte_vec;

/*
 Usage:
 bon_byte_vec vec = {0,0,0};
 bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAGS_DEFAULT);
 write_bon( B );
 bon_w_close_doc( B );
 use( vec.data );
 free(vec.data);
 */
bon_bool bon_vec_writer(void* userData, const void* data, uint64_t nbytes);
	

/*
 Usage:
 FILE* fp = fopen(FILE_NAME, "wb");
 bon_w_doc* B = bon_w_new_doc(&bon_file_writer, fp, BON_W_FLAGS_DEFAULT);
 write_bon( B );
 bon_w_close_doc( B );
 fclose( fp );
*/
bon_bool bon_file_writer(void* user, const void* data, uint64_t nbytes);



//------------------------------------------------------------------------------
// BON writer API

/*
 Low-level API for outputting BON in a serial fashion.
 You must take care to create a correct BON file. In particular:
 
 Match  bon_w_begin_obj    with  bon_w_end_obj
 Match  bon_w_begin_list   with  bon_w_end_list
 Match  bon_w_begin_block  with  bon_w_end_block  (or use bon_w_block)
 
 Make sure bon_w_block_ref references a block with a larger block_id.
 
 Interleave keys and values inside of calls to bon_w_begin_obj bon_w_end_obj
 */


/*
 A writer will be called upon to write a BON-doc in small increments.
 It can be connected to a file-writer, memory-writer, socket etc.
 To signal an error, return false. BON_ERR_WRITE_ERROR will be set in the bon_w_doc.
 */
typedef bon_bool (*bon_w_writer_t)(void* userData, const void* data, uint64_t nbytes);


typedef struct bon_w_doc bon_w_doc;


typedef enum {
	BON_W_FLAG_DEFAULT             =  0,
	BON_W_FLAG_CRC                 =  1 << 0,
	BON_W_FLAG_SKIP_HEADER_FOOTER  =  1 << 1
} bon_w_flags;


// Top level structure
bon_w_doc*   bon_w_new_doc    (bon_w_writer_t writer, void* userData, bon_w_flags flags);
void         bon_w_flush      (bon_w_doc* B);  // Flush writes to the writer

// Writes footer and flushes. Returns final error (if any)
bon_error    bon_w_close_doc  (bon_w_doc* B);

void                       bon_w_set_error  (bon_w_doc* B, bon_error err);
BON_INLINE bon_error    bon_w_error      (bon_w_doc* B);

void         bon_w_begin_block  (bon_w_doc* B, bon_block_id block_id);  // open-ended
void         bon_w_end_block    (bon_w_doc* B);
void         bon_w_block        (bon_w_doc* B, bon_block_id block_id, const void* data, bon_size nbytes);


// The different types of values:
static void  bon_w_begin_obj   (bon_w_doc* B);
static void  bon_w_end_obj     (bon_w_doc* B);

// you must write a value right after this.
static void  bon_w_key         (bon_w_doc* B, const char* utf8);

static void  bon_w_begin_list  (bon_w_doc* B);
static void  bon_w_end_list    (bon_w_doc* B);

static void  bon_w_block_ref  (bon_w_doc* B, bon_block_id id);

static void  bon_w_nil        (bon_w_doc* B);
static void  bon_w_bool       (bon_w_doc* B, bon_bool val);
static void  bon_w_string     (bon_w_doc* B, const char* utf8, bon_size nbytes);
static void  bon_w_cstring    (bon_w_doc* B, const char* utf8); // Zero-ended

// Numbers:
static void  bon_w_uint64     (bon_w_doc* B, uint64_t val);
static void  bon_w_sint64     (bon_w_doc* B, int64_t val);
static void  bon_w_float      (bon_w_doc* B, float val);
static void  bon_w_double     (bon_w_doc* B, double val);


// Writing coherent data
// nbytes == number of bytes implied by 'type', but required for extra safety
void         bon_w_pack    (bon_w_doc* B, const void* data, bon_size nbytes,
								    bon_type* type);

void         bon_w_pack_fmt(bon_w_doc* B, const void* data, bon_size nbytes,
								    const char* fmt, ...);

// Helper:
void         bon_w_pack_array(bon_w_doc* B, const void* data, bon_size nbytes,
										bon_size n_elem, bon_type_id type);



//------------------------------------------------------------------------------
// BON reader API


// Helper function for reading the entire contents of a file:
uint8_t* bon_read_file(bon_size* out_size, const char* path);


typedef struct bon_value  bon_value;
typedef struct bon_r_doc  bon_r_doc;


typedef enum {
	BON_R_FLAG_DEFAULT      =  0,
	
	// Will trigger BON_ERR_CRC If the BON file has no CRC, or it is incorrect.
	BON_R_FLAG_REQUIRE_CRC  =  1 << 0
} bon_r_flags;


// Will parse a BON file. use bon_r_error to query success.
bon_r_doc*  bon_r_open  (const uint8_t* data, bon_size nbytes, bon_r_flags flags);
void        bon_r_close (bon_r_doc* B);
bon_value*  bon_r_root  (bon_r_doc* B); // Access the root object
bon_error   bon_r_error (bon_r_doc* B);


//------------------------------------------------------------------------------

// The logical type of a bon value.
typedef enum {
	BON_LOGICAL_ERROR,  // If the given value was NULL
	BON_LOGICAL_NIL,
	BON_LOGICAL_BOOL,
	BON_LOGICAL_UINT,
	BON_LOGICAL_SINT,
	BON_LOGICAL_DOUBLE,
	BON_LOGICAL_STRING,
	BON_LOGICAL_LIST,
	BON_LOGICAL_OBJECT
} bon_logical_type;

bon_logical_type  bon_r_value_type(bon_r_doc* B, bon_value* val);

// Inspeciting values. Use these before attemting to read a value.
// Passing NULL as 'val' will make all these functions return BON_FALSE.

// val==NULL  ->  BON_FALSE;
bon_bool  bon_r_is_nil     (bon_r_doc* B, bon_value* val);  // note: bon_r_is_nil(B,NULL) == BON_FALSE
bon_bool  bon_r_is_bool    (bon_r_doc* B, bon_value* val);
bon_bool  bon_r_is_int     (bon_r_doc* B, bon_value* val);  // signed or unsigned
bon_bool  bon_r_is_number  (bon_r_doc* B, bon_value* val);  // int or double
bon_bool  bon_r_is_double  (bon_r_doc* B, bon_value* val);  // Specifically double
bon_bool  bon_r_is_string  (bon_r_doc* B, bon_value* val);
bon_bool  bon_r_is_list    (bon_r_doc* B, bon_value* val);
bon_bool  bon_r_is_object  (bon_r_doc* B, bon_value* val);



//------------------------------------------------------------------------------
// Reading values.

// Returns true iff 'val' is a bool and is true. False on fail. No implicit conversion.
bon_bool  bon_r_bool    (bon_r_doc* B, bon_value* val);

// Will return zero if of the wrong type. Double/ints are converted to each other.
uint64_t         bon_r_uint    (bon_r_doc* B, bon_value* val);
int64_t          bon_r_int     (bon_r_doc* B, bon_value* val);
static float     bon_r_float   (bon_r_doc* B, bon_value* val);
static double    bon_r_double  (bon_r_doc* B, bon_value* val);

// Returns NULL if wrong type
const char*  bon_r_cstr  (bon_r_doc* B, bon_value* val);  // zero-ended UTF-8
bon_size     bon_r_strlen(bon_r_doc* B, bon_value* val);  // in bytes (UTF-8)

// Returns 0 if it is not a list.
bon_size    bon_r_list_size(bon_r_doc* B, bon_value* list);
static bon_value*  bon_r_list_elem(bon_r_doc* B, bon_value* list, bon_size ix);

// Number of keys-value pairs in an object
bon_size     bon_r_obj_size(bon_r_doc* B, bon_value* val);

// Return NULL if 'val' is not an object, or ix is out of range.
const char*  bon_r_obj_key  (bon_r_doc* B, bon_value* val, bon_size ix);
bon_value*   bon_r_obj_value(bon_r_doc* B, bon_value* val, bon_size ix);

// Returns NULL if 'val' is not an object or does not have the given key.
bon_value*  bon_r_get_key(bon_r_doc* B, bon_value* val, const char* key);


//------------------------------------------------------------------------------
/*
 API for quickly reading aggregates:
 Any BON value can be read with the 'normal' functions above.
 However, ff the BON encoder wrote data in "aggregate" format,
 the following may prove to be faster, and _much_ faster for large amounts of data.
*/

/* 
 Can read to:      BON_BOOL, const char*, ints, floats, structs
 Can NOT read to:  lists
 
 If the type is a perfect match, bon_r_unpack may do a simple memcpy.
 If not, it will try to convert the values (double->int etc).
 
 return BON_FALSE if there is a mismatch in types,
 array sizes or any missing object keys.
 */
bon_bool bon_r_unpack(bon_r_doc* B, bon_value* srcVal,
                      void* dst, bon_size nbytes,
                      const bon_type* dstType);

// Convenience:
bon_bool bon_r_unpack_fmt(bon_r_doc* B, bon_value* srcVal,
                          void* dst, bon_size nbytes, const char* fmt, ...);

/*
 bon_r_unpack_ptr_fmt is by far the fastest way to read data, but it only works if the format is EXACTLY right. If it is, bon_r_unpack_ptr_fmt returns a pointer to the data. Else, NULL.
 
 Even if bon_r_raw_fmt returns NULL, bon_r_unpack_fmt may work with the same arguments.
 This is because bon_r_unpack_fmt can do value conversions, ignore extra keys in objects etc.
 
 Examples:
 
 const int n = bon_r_list_size(B, val)
 const float* src = bon_r_unpack_ptr_fmt(B, val, n * sizeof(float), "[#f]", n);
 
 struct Vert { float pos[3]; uint8_t color[4]; }
 const int n = bon_r_list_size(B, val)
 const Vert* verts = bon_r_unpack_ptr_fmt(B, val, n * sizeof(Vert),
                                          "[#{$[3f]$[4u8]}]", n, "pos", "color");
 
 */
const void* bon_r_unpack_ptr    (bon_r_doc* B, bon_value* srcVal,
										   bon_size nbytes, const bon_type* dstType);
const void* bon_r_unpack_ptr_fmt(bon_r_doc* B, bon_value* srcVal,
										   bon_size nbytes, const char* fmt, ...);

// Quick access to a pointer e.g. pointer of bytes or floats
const void* bon_r_unpack_array(bon_r_doc* B, bon_value* srcVal,
									    bon_size nelem, bon_type_id type);


//------------------------------------------------------------------------------


// Quick and dirty printout of a value, in json-like format (but NOT json conforming).
// Useful for debugging.
void bon_print(bon_r_doc* B, bon_value* value, FILE* out, size_t indent);

//------------------------------------------------------------------------------

#include "inline.h"

#endif
