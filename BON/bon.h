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


//////////////////////////////////////////////////////////
// BON common

typedef char bon_bool;

#define BON_TRUE  ((bon_bool)1)
#define BON_FALSE ((bon_bool)0)

typedef uint64_t bon_size;
#define BON_ZERO_ENDED (bon_size)(-1)  // Valid as argument of string size

typedef enum {
	BON_SUCCESS = 0,
	
	// Write errors
	BON_ERR_WRITE_ERROR,        // Writer refused
	BON_ERR_BAD_AGGREGATE_SIZE, // bon_w_aggregate got data of the wrong size
	
	// Read errors
	BON_ERR_TOO_SHORT,   // Premature end of file
	BON_ERR_BAD_HEADER,
	BON_ERR_BAD_VLQ,
	BON_ERR_MISSING_LIST_END,
	BON_ERR_MISSING_OBJ_END,
	BON_ERR_BAD_CTRL,
	BON_ERR_KEY_NOT_STRING,
	BON_ERR_BAD_AGGREGATE_TYPE,
	BON_ERR_BAD_TYPE,
	BON_ERR_STRING_NOT_ZERO_ENDED,
	BON_ERR_MISSING_TOKEN,
	
	BON_ERR_TRAILING_DATA,  // Data trailing the document
	
	// bw_read_aggregate etc:
	BON_ERR_NARROWING,
	BON_ERR_NULL_OBJ,
	
	BON_NUM_ERR
} bon_error;

/* The control codes are all in [0x08, 0x10)
 */
typedef enum {
	BON_CTRL_BLOCK_REF   = '@',    BON_CTRL_STRING_VLQ  = '\'',  // 0x40  0x60
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


//////////////////////////////////////////////////////////
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

typedef struct bon_type bon_type;
typedef struct bon_type_tuple bon_type_tuple;

typedef struct bon_type_array bon_type_array;
struct bon_type_array {
	bon_size size;
	bon_type* type;
};

/*
struct bon_type_tuple {
	bon_size size;
	bon_type** types;
};
 */

typedef struct bon_type_struct bon_type_struct;
struct bon_type_struct {
	bon_size     size;
	const char** keys;   // array of utf8 strings
	bon_type**   types;  // array of types
};

struct bon_type {
	bon_type_id id;
	
	// Further info (if applicable)
	union {
		bon_type_array*  array;
		//bon_type_tuple*  tuple;
		bon_type_struct* strct;
	} u;
};


void      bon_free_type(bon_type* t);

// Functions for creating these types
bon_type* bon_new_type_simple(bon_type_id id);
bon_type* bon_new_type_simple_array(bon_size n, bon_type_id id);
bon_type* bon_new_type_array(bon_size n, bon_type* type);
bon_type* bon_new_type_struct(bon_size n, const char** names, bon_type** types);

/*
 struct { float pos[3]; uint8_t color[4]; }
 bon_type* bon_new_type_fmt("{[3f][4u8]}", "pos", "color")
 
 n                - null
 b                - bool
 k                - key (given in vargs)
 u8 u16 u32 u64   - unsigned
 s8 s16 s32 s64   - signed
 i                - int
 ui               - unsigned int
 s                - const char*
 f                - float
 d                - double
 
 captials means "convert to this type from other types"
 
 NULL on fail
 */
bon_type* bon_new_type_fmt(const char* fmt, ...);

bon_size bon_aggregate_payload_size(const bon_type* type);

//////////////////////////////////////////////////////////

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
 bon_w_doc* doc = bon_w_new_doc(&bon_file_writer, fp);
 write_bon( bon );
 bon_w_close_doc( &bon );
 fclose( fp );
*/
bon_bool bon_file_writer(void* user, const void* data, uint64_t nbytes);

//////////////////////////////////////////////////////////
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


typedef struct {
	bon_w_writer_t  writer;
	void*           userData;  // Sent to writer
	bon_error       error;     // If any
	
	// For buffered writing
	uint8_t*  buff;
	unsigned  buff_ix;
	unsigned  buff_size;
} bon_w_doc;


typedef struct {
	uint64_t payloadSize; // in bytes
	uint64_t descSize;    // Size of following BON-desc
	uint8_t desc[];
} bon_w_aggr_type_t;


bon_error      on_get_error(bon_w_doc* doc);

// Top level structure
bon_w_doc*   bon_w_new_doc                     (bon_w_writer_t, void* userData);
void         bon_w_flush                       (bon_w_doc* doc); 
void         bon_w_close_doc                   (bon_w_doc* doc);
void         bon_w_header                (bon_w_doc* doc);  // Should be the first thing written, if written at all.
void         bon_w_footer                (bon_w_doc* doc);  // Should be the last thing written, if written at all.
void         bon_w_block                 (bon_w_doc* doc, uint64_t block_id, const void* data, bon_size nbytes);
void         bon_w_declare_root_block_unsized  (bon_w_doc*);

// The different types of values:
void         bon_w_block_ref  (bon_w_doc* doc, uint64_t block_id);
void         bon_w_begin_obj        (bon_w_doc* doc);
void         bon_w_end_obj          (bon_w_doc* doc);
void         bon_w_key              (bon_w_doc* doc, const char* utf8, bon_size nbytes);  // you must write a value right after this
void         bon_w_begin_list       (bon_w_doc* doc);
void         bon_w_end_list         (bon_w_doc* doc);

void         bon_w_nil        (bon_w_doc* doc);
void         bon_w_bool       (bon_w_doc* doc, bon_bool val);
void         bon_w_string     (bon_w_doc* doc, const char* utf8, bon_size nbytes);
void         bon_w_uint64     (bon_w_doc* doc, uint64_t val);
void         bon_w_sint64     (bon_w_doc* doc, int64_t val);
void         bon_w_float      (bon_w_doc* doc, float val);
void         bon_w_double     (bon_w_doc* doc, double val);
void         bon_w_aggregate  (bon_w_doc* doc, bon_type* type, const void* data, bon_size nbytes);             // Number of bytes implied by 'type', but required for extra safety
void         bon_w_array      (bon_w_doc* doc, bon_size n_elem, bon_type_id type, const void* data, bon_size nbytes);      // Helper

bon_w_aggr_type_t*  bon_w_begin_type_simple(bon_ctrl t);
bon_w_aggr_type_t*  bon_w_new_type_simple_array(bon_size n, bon_ctrl t);
bon_w_aggr_type_t*  bon_w_new_type_array(bon_size n, bon_w_aggr_type_t* c);          // "An array of several of these"
bon_w_aggr_type_t*  bon_w_new_type_tuple(bon_size n, const bon_w_aggr_type_t** c);   // c is an array of n types
void                     bon_w_free_type(bon_w_aggr_type_t** type);                       // Will NOT free nested types


//////////////////////////////////////////////////////////
// BON bon_reader API


typedef enum {
	BON_VALUE_NIL,
	BON_VALUE_BOOL,
	BON_VALUE_UINT64,
	BON_VALUE_SINT64,
	BON_VALUE_DOUBLE,
	BON_VALUE_STRING,
	BON_VALUE_LIST,
	BON_VALUE_OBJ,
	BON_VALUE_AGGREGATE,
	BON_VALUE_BLOCK_REF
} bon_value_type;


typedef struct bon_value bon_value;
typedef struct bon_kv bon_kv;
typedef struct bon_r_block_t bon_r_block_t;


typedef struct {
	bon_size   size;
	bon_size   cap;
	bon_value* data;
} bon_values;


typedef struct {
	bon_size  size;
	bon_size  cap;
	bon_kv*   data;
} bon_kvs;


typedef struct {
	bon_size        size;  // Size in bytes, excluding trailing zero.
	const uint8_t*  ptr;   // Points to 'size' bytes of an utf8 encoded string, followed by a zero.
} bon_value_str;


typedef struct {
	bon_values          values;
} bon_value_list;


typedef struct {
	bon_type        type;
	const uint8_t*  data;
} bon_value_agg;


typedef struct {
	bon_kvs  kvs;
} bon_value_obj;


typedef union {
	bon_bool        boolean;
	uint64_t        u64;
	int64_t         s64;
	double          dbl;
	bon_value_str   str;
	bon_value_list  list;
	bon_value_obj   obj;
	bon_value_agg   agg;
	uint64_t        block_id;
} bon_value_union;


struct bon_value {
	bon_value_type  type;
	bon_value_union u;
};


struct bon_kv {
	bon_value key; // will be a string
	bon_value val;
};
	

/* Low level statistics about a bon-file. */
typedef struct {
	bon_size bytes_file;         // Number of bytes in the entire file
	
	bon_size count_string;
	bon_size bytes_string_dry;   // Number of bytes taken up by strings (excluding fluff).
	bon_size bytes_string_wet;   // Number of bytes taken up by strings (including header and zero byte).
	
	bon_size count_aggr;
	bon_size bytes_aggr_dry;     // Number of bytes taken up by strings (excluding header).
	bon_size bytes_aggr_wet;     // Number of bytes taken up by strings (including header).
} bon_stats;

typedef struct {
	bon_size   id;              // Id of block
	//bon_size   payload_offset;  // Byte offset in document to payload start
	bon_size   payload_size;    // Byte size of payload. 0 means unknown.
	bon_bool   parsed;          // if false, 'value' is not yet valid.
	bon_value  value;
} bon_r_block;

typedef struct {
	bon_size      size;
	bon_size      cap;
	bon_r_block*  data;
} bon_r_blocks;

typedef struct {
	bon_error      error;       // If any
	bon_r_blocks   blocks;
	bon_stats      stats;       // Info about the read file
} bon_r_doc;


// Will parse a BON file.
bon_r_doc* bon_r_open(const uint8_t* data, bon_size nbytes);
void       bon_r_close(bon_r_doc* doc);

// Returns NULL on fail
const bon_value* bon_r_get_block(const bon_r_doc* doc, uint64_t block_id);


const bon_value* bon_r_root(const bon_r_doc* doc);


const bon_value* bon_r_get_key(const bon_value* obj, const char* key);
	
// Convenicence: returns NULL if wrong type
const char* bon_r_cstr(const bon_value* obj);
	
// Convenicence: sets an error if incompatible type
uint64_t bon_r_uint(bon_r_doc* doc, const bon_value* obj);

	
// Returns 0 if it is not a list.
bon_size          bon_r_list_size(const bon_value* list);
const bon_value*  bon_r_list_elem(const bon_value* list, bon_size ix);

/* 
 Can read to:
 BON_BOOL,
 const char*,
 int,
 ..
 
 Can NOT read to:
 lists
 
 False on fail
 */
bon_bool bon_r_read_aggregate(const bon_r_doc* doc, const bon_value* srcVal, const bon_type* dstType, void* dst, bon_size nbytes);

// Convenience:
bon_bool bon_r_read_aggregate_fmt(const bon_r_doc* doc, const bon_value* srcVal,
											 void* dst, bon_size nbytes, const char* fmt);

//////////////////////////////////////////////////////////

// Quick and dirty printout of a value, in json-like format, but NOT json conforming.
void bon_print(FILE* out, const bon_value* value, size_t indent);


//////////////////////////////////////////////////////////
// Helper for reading a binary stream without overflowing:

typedef struct {
	const uint8_t*  data;
	bon_size        nbytes;
	bon_error       error;
	bon_r_doc*      doc;     // For stats. May be null.
} bon_reader;


/* Read a simple value denoted by 't', and interpret is as a signed int. */
int64_t br_read_sint64(bon_reader* br, bon_ctrl t);

/* Read a simple value denoted by 't', and interpret is as an unsigned int. */
uint64_t br_read_uint64(bon_reader* br, bon_ctrl t);

/* Read a simple value denoted by 't', and interpret is as a double. */
double br_read_double(bon_reader* br, bon_ctrl t);


#endif
