//
//  bon.h
//  BON
//
//  Created by emilk on 2013-02-01.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#ifndef BON_bon_w_h
#define BON_bon_w_h

#include <stdint.h>
#include <stdio.h>


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

typedef uint64_t bon_size;
#define BON_ZERO_ENDED (bon_size)(-1)  // Valid as argument of string size

typedef uint64_t bon_block_id;
#define BON_BAD_BLOCK_ID (bon_block_id)(-1)


//------------------------------------------------------------------------------


typedef enum {
	BON_SUCCESS = 0,
	
	// Write errors
	BON_ERR_WRITE_ERROR,        // Writer refused
	BON_ERR_BAD_AGGREGATE_SIZE, // bon_w_aggr got data of the wrong size
	
	// Read errors
	BON_ERR_TOO_SHORT,   // Premature end of file
	BON_ERR_BAD_HEADER,
	BON_ERR_BAD_VLQ,
	BON_ERR_MISSING_LIST_END,
	BON_ERR_MISSING_OBJ_END,
	BON_ERR_BAD_CTRL,
	BON_ERR_BAD_KEY,
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


/* A code byte is divided into several ranges of values.
 This enum specifies those ranges.
 */
typedef enum {
	BON_SHORT_POS_INT_START     = 0,
	BON_SHORT_POS_INT_COUNT     = 32,
	
	BON_SHORT_STRING_START      = 32,
	BON_SHORT_STRING_COUNT      = 32,
	
	BON_SHORT_CODES_START       = 64,  // Control codes - see bon_ctrl
	BON_SHORT_CODES_COUNT       = 64,
	
	BON_SHORT_BLOCK_START       = 128,
	BON_SHORT_BLOCK_COUNT       = 64,
	BON_SHORT_BLOCK_END         = BON_SHORT_BLOCK_START + BON_SHORT_BLOCK_COUNT,
	
	BON_SHORT_ARRAY_START       = BON_SHORT_BLOCK_END,
	BON_SHORT_ARRAY_COUNT       = 16,
	
	BON_SHORT_BYTE_ARRAY_START  = BON_SHORT_ARRAY_START + BON_SHORT_ARRAY_COUNT,
	BON_SHORT_BYTE_ARRAY_COUNT  = 16,
	
	BON_SHORT_STRUCT_START      = BON_SHORT_BYTE_ARRAY_START + BON_SHORT_BYTE_ARRAY_COUNT,
	BON_SHORT_STRUCT_COUNT      = 16,
	
	BON_SHORT_NEG_INT_START     = BON_SHORT_STRUCT_START + BON_SHORT_STRUCT_COUNT,
	BON_SHORT_NEG_INT_COUNT     = 16,
	
	BON_SHORT_AGGREGATES_START  = BON_SHORT_ARRAY_START
} bon_compressed_ranges;

#ifdef DEBUG
#  define BON_COMPRESS_(start, count, n)  (assert(n < count), (uint8_t)((start) + n))
#else
#  define BON_COMPRESS_(start, count, n)  (uint8_t)((start) + n)
#endif

#define BON_COMPRESS(prefix, n)  BON_COMPRESS_(prefix ## START, prefix ## COUNT, n)

#define BON_SHORT_STRING(n)       BON_COMPRESS(BON_SHORT_STRING_,       n)
#define BON_SHORT_BLOCK(n)        BON_COMPRESS(BON_SHORT_BLOCK_,        n)
#define BON_SHORT_ARRAY(n)        BON_COMPRESS(BON_SHORT_ARRAY_,        n)
#define BON_SHORT_BYTE_ARRAY(n)   BON_COMPRESS(BON_SHORT_BYTE_ARRAY_,   n)
#define BON_SHORT_STRUCT(n)       BON_COMPRESS(BON_SHORT_STRUCT_,       n)

//------------------------------------------------------------------------------


/* The control codes are all in [0x08, 0x10)
 */
typedef enum {
	BON_CTRL_BLOCK_REF   = 0x40,   BON_CTRL_STRING_VLQ  = 0x60,  // @  `
	BON_CTRL_ARRAY_VLQ   = 'A',
	BON_CTRL_HEADER      = 'B',
	BON_CTRL_TUPLE_VLQ   = 'C',    BON_CTRL_STRUCT_VLQ  = 'c',
	BON_CTRL_BLOCK_BEGIN = 'D',    BON_CTRL_BLOCK_END   = 'd',
	BON_CTRL_TRUE        = 'E',    BON_CTRL_FALSE       = 'e',
	//BON_CTRL_FOOTER      = 'F',
	//BON_CTRL_BLOCK_INDEX = 'I',
	//BON_CTRL_LIST_VLQ    = 'L',
	BON_CTRL_NIL        = 'N',
	//BON_CTRL_OBJECT_VLQ  = 'O',
	
	BON_CTRL_SINT8       = 'P',
	BON_CTRL_UINT8       = 'Q',
	BON_CTRL_SINT16_LE   = 'R',    BON_CTRL_SINT16_BE   = 'r',
	BON_CTRL_UINT16_LE   = 'S',    BON_CTRL_UINT16_BE   = 's',
	BON_CTRL_SINT32_LE   = 'T',    BON_CTRL_SINT32_BE   = 't',
	BON_CTRL_UINT32_LE   = 'U',    BON_CTRL_UINT32_BE   = 'u',
	BON_CTRL_SINT64_LE   = 'V',    BON_CTRL_SINT64_BE   = 'v',
	BON_CTRL_UINT64_LE   = 'W',    BON_CTRL_UINT64_BE   = 'w',
	
	BON_CTRL_FLOAT32_LE  = 'X',    BON_CTRL_FLOAT32_BE  = 'x',
	BON_CTRL_FLOAT64_LE  = 'Y',    BON_CTRL_FLOAT64_BE  = 'y',
	
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
	BON_CTRL_FLOAT32  = BON_CTRL_FLOAT32_LE,
	BON_CTRL_FLOAT64  = BON_CTRL_FLOAT64_LE,
#else
	BON_CTRL_SINT16   = BON_CTRL_SINT16_BE,
	BON_CTRL_UINT16   = BON_CTRL_UINT16_BE,
	BON_CTRL_SINT32   = BON_CTRL_SINT32_BE,
	BON_CTRL_UINT32   = BON_CTRL_UINT32_BE,
	BON_CTRL_SINT64   = BON_CTRL_SINT64_BE,
	BON_CTRL_UINT64   = BON_CTRL_UINT64_BE,
	BON_CTRL_FLOAT32  = BON_CTRL_FLOAT32_BE,
	BON_CTRL_FLOAT64  = BON_CTRL_FLOAT64_BE
#endif
} bon_ctrl;


//------------------------------------------------------------------------------
/*
 The general type system.
 This is used for reading and writing.
 For writing, the user may spec up a type and write it.
 For reading, the user may spec up a type and ask BON to read it.
 For reading, the user may inquire BON about the type of the next value.
 */

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
	
	BON_TYPE_FLOAT32_LE = BON_CTRL_FLOAT32_LE,
	BON_TYPE_FLOAT32_BE = BON_CTRL_FLOAT32_BE,
	BON_TYPE_FLOAT64_LE = BON_CTRL_FLOAT64_LE,
	BON_TYPE_FLOAT64_BE = BON_CTRL_FLOAT64_BE,
	
	BON_TYPE_SINT16   = BON_CTRL_SINT16,
	BON_TYPE_UINT16   = BON_CTRL_UINT16,
	BON_TYPE_SINT32   = BON_CTRL_SINT32,
	BON_TYPE_UINT32   = BON_CTRL_UINT32,
	BON_TYPE_SINT64   = BON_CTRL_SINT64,
	BON_TYPE_UINT64   = BON_CTRL_UINT64,
	BON_TYPE_FLOAT32  = BON_CTRL_FLOAT32,
	BON_TYPE_FLOAT64  = BON_CTRL_FLOAT64
} bon_type_id;


//------------------------------------------------------------------------------


typedef enum {
	BON_FLAG_DEFAULT      =  0,
	BON_FLAG_NO_COMPRESS  =  1 << 0  // Turn off compression of small integers, string, arrays etc
} bon_flags;


//------------------------------------------------------------------------------


typedef struct bon_type bon_type;


void      bon_free_type(bon_type* t);

// Functions for creating these types
bon_type* bon_new_type_simple(bon_type_id id);
bon_type* bon_new_type_simple_array(bon_size n, bon_type_id id);
bon_type* bon_new_type_array(bon_size n, bon_type* type);

/*
 struct Vert { float pos[3]; uint8_t color[4]; }
 bon_type* bon_new_type_fmt("{[3f][4u8]}", "pos", "color")
 
 b                - bon_bool (single byte)
 u8 u16 u32 u64   - unsigned
 i8 i16 i32 i64   - signed
 f                - float
 d                - double
 s                - const char*
 {...}            - object with any number of types (keys given in vargs)
 [3f]             - array of three floats
 [#i]             - array of signed integers (size given in vargs as bon_size)
  
 NULL on fail
 */
bon_type* bon_new_type_fmt(const char* fmt, ...);

bon_size  bon_aggregate_payload_size(const bon_type* type);

// Returns true if the two types are exactly equal
bon_bool  bon_type_eq(const bon_type* a, const bon_type* b);


//------------------------------------------------------------------------------


typedef struct {
	bon_size   size;
	bon_size   cap;
	uint8_t*   data;
} bon_byte_vec;

/*
 Usage:
 bon_byte_vec vec = {0,0,0};
 bon_w_doc* bon = bon_w_new_doc(bon_vec_writer, &vec);
 write_bon( bon );
 bon_w_close_doc( &bon );
 use( vec.data );
 free(vec.data);
 */
bon_bool bon_vec_writer(void* userData, const void* data, uint64_t nbytes);
	

/*
 Usage:
 FILE* fp = fopen(FILE_NAME, "wb");
 bon_w_doc* B = bon_w_new_doc(&bon_file_writer, fp);
 write_bon( bon );
 bon_w_close_doc( &bon );
 fclose( fp );
*/
bon_bool bon_file_writer(void* user, const void* data, uint64_t nbytes);



//------------------------------------------------------------------------------
// BON writer API

/*
 Low-level API for outputting BON in a serial fashion.
 The order you call these are very important.
 */


/*
 A writer will be called upon to write a BON-doc in small increments.
 It can be connected to a file-writer, memory-writer, socket etc.
 Return false on fail, and BON_ERR_WRITE_ERROR will be set
 */
typedef bon_bool (*bon_w_writer_t)(void* userData, const void* data, uint64_t nbytes);


typedef struct bon_w_doc bon_w_doc;


void         bon_w_set_error(bon_w_doc* B, bon_error err);
bon_error    bon_w_error(bon_w_doc* B);

// Top level structure
bon_w_doc*   bon_w_new_doc    (bon_w_writer_t, void* userData, bon_flags flags);
void         bon_w_flush      (bon_w_doc* B); 
void         bon_w_close_doc  (bon_w_doc* B);
void         bon_w_header     (bon_w_doc* B);  // Should be the first thing written, if written at all.
void         bon_w_footer     (bon_w_doc* B);  // Should be the last thing written, if written at all.

void         bon_w_block_ref    (bon_w_doc* B, uint64_t block_id);
void         bon_w_begin_block  (bon_w_doc* B, uint64_t block_id);  // open-ended
void         bon_w_end_block    (bon_w_doc* B);
void         bon_w_block        (bon_w_doc* B, uint64_t block_id, const void* data, bon_size nbytes);


// The different types of values:
void         bon_w_block_ref   (bon_w_doc* B, uint64_t block_id);
void         bon_w_begin_obj   (bon_w_doc* B);
void         bon_w_end_obj     (bon_w_doc* B);
void         bon_w_key         (bon_w_doc* B, const char* utf8, bon_size nbytes);  // you must write a value right after this
void         bon_w_begin_list  (bon_w_doc* B);
void         bon_w_end_list    (bon_w_doc* B);

void         bon_w_nil        (bon_w_doc* B);
void         bon_w_bool       (bon_w_doc* B, bon_bool val);
void         bon_w_string     (bon_w_doc* B, const char* utf8, bon_size nbytes);
void         bon_w_uint64     (bon_w_doc* B, uint64_t val);
void         bon_w_sint64     (bon_w_doc* B, int64_t val);
void         bon_w_float      (bon_w_doc* B, float val);
void         bon_w_double     (bon_w_doc* B, double val);

// Writing coherent data
// nbytes == number of bytes implied by 'type', but required for extra safety
void         bon_w_aggr    (bon_w_doc* B, const void* data, bon_size nbytes,
								    bon_type* type);

void         bon_w_aggr_fmt(bon_w_doc* B, const void* data, bon_size nbytes,
								    const char* fmt, ...);

// Helper:
void         bon_w_array      (bon_w_doc* B, bon_size n_elem, bon_type_id type,
										 const void* data, bon_size nbytes);



//------------------------------------------------------------------------------
// BON bon_reader API


typedef struct bon_value  bon_value;
typedef struct bon_r_doc  bon_r_doc;


//------------------------------------------------------------------------------
// Public API:

// Will parse a BON file. use bon_r_error to query success.
bon_r_doc*  bon_r_open  (const uint8_t* data, bon_size nbytes);
void        bon_r_close (bon_r_doc* B);
bon_value*  bon_r_root  (bon_r_doc* B);
bon_error   bon_r_error (bon_r_doc* B);


//------------------------------------------------------------------------------
// Inspeciting values. Use these before attemting to read a value.
// Passing NULL as 'val' will make all these functions return BON_FALSE.

// val==NULL  ->  BON_FALSE;
bon_bool  bon_r_is_nil     (bon_r_doc* B, bon_value* val);

bon_bool  bon_r_is_bool    (bon_r_doc* B, bon_value* val);

// signed or unsigned
bon_bool  bon_r_is_int     (bon_r_doc* B, bon_value* val);

// int or double:
bon_bool  bon_r_is_number  (bon_r_doc* B, bon_value* val);

// Specifically double
bon_bool  bon_r_is_double  (bon_r_doc* B, bon_value* val);

bon_bool  bon_r_is_string  (bon_r_doc* B, bon_value* val);

bon_bool  bon_r_is_list    (bon_r_doc* B, bon_value* val);

bon_bool  bon_r_is_object  (bon_r_doc* B, bon_value* val);


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

//------------------------------------------------------------------------------
// Reading values.

// Returns true iff 'val' is a bool and is true. False on fail. No implicit conversion.
bon_bool  bon_r_bool    (bon_r_doc* B, bon_value* val);

// Will return zeroif of the wrong type. Double/ints are converted to each other.
uint64_t  bon_r_uint    (bon_r_doc* B, bon_value* val);
int64_t   bon_r_int     (bon_r_doc* B, bon_value* val);
double    bon_r_double  (bon_r_doc* B, bon_value* val);
float     bon_r_float   (bon_r_doc* B, bon_value* val);

// Returns NULL if wrong type
bon_size     bon_r_strlen(bon_r_doc* B, bon_value* val);
const char*  bon_r_cstr  (bon_r_doc* B, bon_value* val);

// Returns 0 if it is not a list.
bon_size    bon_r_list_size(bon_r_doc* B, bon_value* list);
bon_value*  bon_r_list_elem(bon_r_doc* B, bon_value* list, bon_size ix);

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
 
 If the type is a perfect match, bon_r_aggr_read may do a simple memcpy.
 If not, it will try to convert the values (double->int etc).
 
 return BON_FALSE if there is a mismatch in types,
 array sizes or any missing object keys.
 */
bon_bool bon_r_aggr_read(bon_r_doc* B, bon_value* srcVal,
								 void* dst, bon_size nbytes,
								 const bon_type* dstType);

// Convenience:
bon_bool bon_r_aggr_read_fmt(bon_r_doc* B, bon_value* srcVal,
											 void* dst, bon_size nbytes, const char* fmt, ...);

/*
 bon_r_aggr_ptr_fmt is by far the fastest way to read data, but it only works if the format is EXACTLY right. If it is, bon_r_aggr_ptr_fmt returns a pointer to the data. Else, NULL.
 
 Even if bon_r_raw_fmt returns NULL, bon_r_aggr_read_fmt may work with the same arguments.
 This is because bon_r_aggr_read_fmt can do value conversions, ignore extra keys in objects etc.
 
 Examples:
 
 const int n = bon_r_list_size(B, val)
 const float* src = bon_r_raw_fmt(B, val, n * sizeof(float), "[#f]", n);
 
 struct Vert { float pos[3]; uint8_t color[4]; }
 const int n = bon_r_list_size(B, val)
 const Vert* verts = bon_r_raw_fmt(B, val, n * sizeof(Vert), "[#{[3f][4u8]}]", n, "pos", "color");
 
 */
const void* bon_r_aggr_ptr    (bon_r_doc* B, bon_value* srcVal,
										 bon_size nbytes, const bon_type* dstType);
const void* bon_r_aggr_ptr_fmt(bon_r_doc* B, bon_value* srcVal,
										 bon_size nbytes, const char* fmt, ...);

// Quick access to a pointer e.g. pointer of bytes or floats
const void* bon_r_array_ptr(bon_r_doc* B, bon_value* srcVal,
									 bon_size nelem, bon_type_id type);


//------------------------------------------------------------------------------


// Quick and dirty printout of a value, in json-like format, but NOT json conforming.
// Useful for debugging.
void bon_print(FILE* out, bon_value* value, size_t indent);



#endif
