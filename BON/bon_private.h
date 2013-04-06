//
//  bon_private.h
//  BON
//
//  Created by emilk on 2013-04-06.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#ifndef BON_bon_private_h
#define BON_bon_private_h

/*
 This is NOT part of the public API.
 Only include this file if you whant to poke around the internals of BON!
 HERE BE DRAGONS
 */



//-----------------------------------------------
// bon_type etc

typedef struct bon_type_array bon_type_array;

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
	bon_size      size;
	const char**  keys;   // array of utf8 strings
	bon_type**    types;  // array of types
};

struct bon_type {
	bon_type_id id;
	
	// Further info (if applicable)
	union {
		bon_type_array*  array;
		bon_type_struct* strct;
	} u;
};


//-----------------------------------------------


struct bon_w_doc {
	bon_w_writer_t  writer;
	void*           userData;  // Sent to writer
	bon_flags       flags;
	bon_error       error;     // If any
	
	// For buffered writing
	uint8_t*  buff;
	bon_size  buff_ix;
	bon_size  buff_size;
};

//-----------------------------------------------



typedef struct bon_kv         bon_kv;


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
	uint64_t        blockRefId;
} bon_value_union;


struct bon_value {
	bon_value_type  type;
	bon_value_union u;
};


/* Low level statistics about a bon-file. */
typedef struct {
	bon_size bytes_file;         // Number of bytes in the entire file
	
	bon_size count_string;
	bon_size bytes_string_dry;   // Number of bytes taken up by strings (excluding fluff).
	//bon_size bytes_string_wet;   // Number of bytes taken up by strings (including header and zero byte).
	
	bon_size count_aggr;
	bon_size bytes_aggr_dry;     // Number of bytes taken up by strings (excluding header).
	//bon_size bytes_aggr_wet;     // Number of bytes taken up by strings (including header).
} bon_stats;

typedef struct {
	bon_size        id;              // Id of block
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
	bon_error      error;       // If any
	bon_r_blocks   blocks;
	bon_stats      stats;       // Info about the read file
};

struct bon_kv {
	bon_value key; // will be a string
	bon_value val;
};


//-----------------------------------------------


#endif
