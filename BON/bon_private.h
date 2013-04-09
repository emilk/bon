//
//  bon_private.h
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#ifndef BON_bon_private_h
#define BON_bon_private_h

/*
 This is NOT part of the public API.
 Only include this file if you want to poke around the internals of BON!
 HERE BE DRAGONS
*/


//------------------------------------------------------------------------------

typedef enum {
	BON_VALUE_NONE       = 0,
	BON_VALUE_NIL        = BON_CTRL_NIL,
	BON_VALUE_BOOL       = BON_TYPE_BOOL,
	BON_VALUE_UINT64     = BON_TYPE_UINT64,
	BON_VALUE_SINT64     = BON_TYPE_SINT64,
	BON_VALUE_DOUBLE     = BON_TYPE_FLOAT64,
	BON_VALUE_STRING     = BON_TYPE_STRING,
	BON_VALUE_LIST       = BON_CTRL_LIST_BEGIN,
	BON_VALUE_OBJ        = BON_CTRL_OBJ_BEGIN,
	BON_VALUE_BLOCK_REF  = BON_CTRL_BLOCK_REF,
	BON_VALUE_AGGREGATE  = 255, // Won't conflict with any of the aboe
} bon_value_type;



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
// bon_type etc

typedef struct bon_type_array  bon_type_array;
typedef struct bon_kt          bon_kt;

struct bon_type_array {
	bon_size   size;
	bon_type*  type;
};

/*
 struct bon_type_tuple {
 bon_size size;
 bon_type** types;
 };
 */

typedef struct bon_type_struct bon_type_struct;
struct bon_type_struct {
	bon_size  size;  // size of 'kts'
	bon_kt*   kts;   // Key-type map
};

struct bon_type {
	bon_type_id id;
	
	// Further info (if applicable)
	union {
		bon_type_array*  array;
		bon_type_struct* strct;
	} u;
};


// key-type pair
struct bon_kt {
	/*
	 Key is in UTF8.
	 It either points to inside of a read BON document
	 or to a string literal supplied by the user
	 */
	const char*  key;
	bon_type     type;
};


//------------------------------------------------------------------------------


struct bon_w_doc {
	bon_w_writer_t  writer;
	void*           userData;  // Sent to writer
	
	// Write buffer:
	uint8_t*     buff;
	bon_size     buff_size;  // size of 'buff
	bon_size     buff_ix;    // usage of 'buff'
	
	uint32_t     crc_inv;    // Accumulator of crc value (if BON_W_FLAG_CRC is set)
	bon_w_flags  flags;
	bon_error    error;      // If any
};

//------------------------------------------------------------------------------



typedef struct bon_kv bon_kv;


typedef struct {
	bon_size  size;
	bon_size  cap;
	bon_kv*   data;
} bon_kvs;


typedef struct {
	bon_size     size;  // Size in bytes, excluding trailing zero.
	const char*  ptr;   // Points to 'size' bytes of an utf8 encoded string, followed by a zero.
} bon_value_str;


typedef struct {
	bon_size   size;
	bon_size   cap;
	bon_value* data;
} bon_value_list;


typedef struct {
	bon_type        type;
	const uint8_t*  data;
	bon_value*      exploded; // if non-NULL, this contains the packed data in explicit form. Lazily calculated iff user queires it.
} bon_value_agg;


typedef struct {
	// move 'kvs' inline?
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
	bon_block_id    blockRefId;
} bon_value_union;


struct bon_value {
	bon_value_type   type;
	bon_value_union  u;
};


/* Low level statistics about a bon-file. */
typedef struct {
	bon_size  bytes_file;          // Number of bytes in the entire file
	
	bon_size  count_string;
	bon_size  bytes_string_dry;    // Number of bytes taken up by strings (excluding fluff).
	//bon_size bytes_string_wet;   // Number of bytes taken up by strings (including header and zero byte).
	
	bon_size  count_aggr;
	bon_size  bytes_aggr_dry;      // Number of bytes taken up by strings (excluding header).
	//bon_size bytes_aggr_wet;     // Number of bytes taken up by strings (including header).
} bon_stats;


typedef struct {
	bon_block_id    id;              // Id of block
	const uint8_t*  payload;         // Pointer into document to payload start
	bon_size        payload_size;    // Byte size of payload. 0 means unknown.
	bon_value       value;           // value, if 'parsed' is true.
	bon_bool        parsed;          // if false, 'value' is not yet valid.
} bon_r_block;


typedef struct {
	bon_size      size;
	bon_size      cap;
	bon_r_block*  data;
} bon_r_blocks;


struct bon_r_doc {
	bon_r_blocks   blocks;
	bon_stats      stats;       // Info about the read file
	bon_error      error;       // If any
	bon_r_flags    flags;
};


struct bon_kv {
	const char*  key;  // UTF8 - points to inside of document
	bon_value    val;
};


//------------------------------------------------------------------------------


// Helper for reading a binary stream without overflowing:

typedef struct {
	const uint8_t*  data;
	bon_size        nbytes;
	bon_r_doc*      B;
	bon_block_id    block_id;  // Current block being read, or BON_BAD_BLOCK_ID.
	bon_error       error;
} bon_reader;


/* Read a simple value denoted by 't', and interpret is as a signed int. */
int64_t br_read_sint64(bon_reader* br, bon_type_id t);

/* Read a simple value denoted by 't', and interpret is as an unsigned int. */
uint64_t br_read_uint64(bon_reader* br, bon_type_id t);

/* Read a simple value denoted by 't', and interpret is as a double. */
double br_read_double(bon_reader* br, bon_type_id t);


//------------------------------------------------------------------------------
// Things common to bon.c, write.c, read.c:

void bon_onError(const char* msg);

bon_type* bon_new_type_fmt_ap(const char** fmt, va_list* ap);

bon_size  bon_aggregate_payload_size(const bon_type* type);
bon_size  bon_struct_payload_size(const bon_type_struct* strct);

void      bon_free_type_insides(bon_type* t);

// Byte size of atomic types
uint64_t  bon_type_size(bon_type_id t);

// Returns NULL on fail
bon_value* bon_r_get_block(bon_r_doc* B, bon_block_id block_id);

bon_value* bon_r_follow_refs(bon_r_doc* B, bon_value* val);

// Endianness conversion (used for crc32)
uint32_t le_to_uint32(uint32_t v);
uint32_t uint32_to_le(uint32_t v);

//------------------------------------------------------------------------------

// TODO: handle failed allocs
#define BON_ALLOC_TYPE(n, type)  (type*)calloc(n, sizeof(type))

#define BON_VECTOR_EXPAND(vec, Type, amnt)                                 \
/**/  (vec).size += amnt;                                                  \
/**/  if ((vec).size > (vec).cap) {                                        \
/**/      size_t newCap = ((vec).size + 2) + (vec).size/2;                 \
/**/      (vec).data = (Type*)realloc((vec).data, newCap * sizeof(Type));  \
/**/      /*memset((Type*)((vec).data) + (vec).cap, 0,*/                       \
/**/      /*       (newCap - (vec).cap)*sizeof(Type));*/                       \
/**/      (vec).cap  = newCap;                                             \
/**/  }



//------------------------------------------------------------------------------


#endif
