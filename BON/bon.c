//
//  bon.c
//  BON
//
//  Created by emilk on 2013-02-01.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//


#include "bon.h"
#include <string.h>  // memset
#include <stdlib.h>  // malloc
#include <stdio.h>   // fprintf(stderr, ...
#include <assert.h>
#include <math.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */


// For printf:ing int64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/////////////////////////////////////

#define ALLOC_TYPE(n, type) (type*)calloc(n, sizeof(type))

#define VECTOR_EXPAND(vec, Type, amnt)                                     \
/**/  (vec).size += amnt;                                                  \
/**/  if ((vec).size > (vec).cap) {                                        \
/**/      size_t newCap = ((vec).size + 2) + (vec).size/2;                 \
/**/      (vec).data = (Type*)realloc((vec).data, newCap * sizeof(Type));  \
/**/      (vec).cap  = newCap;                                             \
/**/  }


/////////////////////////////////////

// Start values for ranges
const uint8_t FIXED_STRING_START      = 32;
const uint8_t FIXED_CODES_START       = 64;
const uint8_t FIXED_BLOCK_START       = 128;
const uint8_t FIXED_ARRAY_START       = FIXED_BLOCK_START  + 64;
const uint8_t FIXED_BYTE_ARRAY_START  = FIXED_ARRAY_START       + 16;
const uint8_t FIXED_STRUCT_START      = FIXED_BYTE_ARRAY_START  + 16;
const uint8_t FIXED_NEG_INT_START     = FIXED_STRUCT_START      + 16;

const uint8_t FIXED_AGGREGATES_START  = FIXED_ARRAY_START;


/////////////////////////////////////


uint64_t bon_w_type_size(bon_type_id t)
{
	switch (t)
	{
		case BON_TYPE_SINT8:
		case BON_TYPE_UINT8:
			return 1;
			
		case BON_TYPE_SINT16_LE:
		case BON_TYPE_SINT16_BE:
		case BON_TYPE_UINT16_LE:
		case BON_TYPE_UINT16_BE:
			return 2;
			
		case BON_TYPE_SINT32_LE:
		case BON_TYPE_SINT32_BE:
		case BON_TYPE_UINT32_LE:
		case BON_TYPE_UINT32_BE:
		case BON_TYPE_FLOAT32_LE:
		case BON_TYPE_FLOAT32_BE:
			return 4;
			
		case BON_TYPE_SINT64_LE:
		case BON_TYPE_SINT64_BE:
		case BON_TYPE_UINT64_LE:
		case BON_TYPE_UINT64_BE:
		case BON_TYPE_FLOAT64_LE:
		case BON_TYPE_FLOAT64_BE:
			return 8;
			
		default:
			fprintf(stderr, "BON: bad type in 'bon_w_type_size'\n");
			return 0;
	}
}

bon_bool bon_is_sint(uint8_t ctrl)
{
	switch (ctrl) {
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE:
			return BON_TRUE;
			
		default:
			return BON_FALSE;
	}
}

bon_bool bon_is_uint(uint8_t ctrl)
{
	switch (ctrl) {
		case BON_CTRL_UINT8:
		case BON_CTRL_UINT16_LE:
		case BON_CTRL_UINT16_BE:
		case BON_CTRL_UINT32_LE:
		case BON_CTRL_UINT32_BE:
		case BON_CTRL_UINT64_LE:
		case BON_CTRL_UINT64_BE:
			return BON_TRUE;
			
		default:
			return BON_FALSE;
	}
}

bon_bool bon_is_float_double(uint8_t ctrl)
{
	switch (ctrl) {
		case BON_CTRL_FLOAT32_LE:
		case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:
		case BON_CTRL_FLOAT64_BE:
			return BON_TRUE;
			
		default:
			return BON_FALSE;
	}
}

bon_bool bon_is_simple_type(uint8_t ctrl)
{
	switch (ctrl) {
		case BON_CTRL_SINT8:
		case BON_CTRL_UINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_UINT16_LE:
		case BON_CTRL_UINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_UINT32_LE:
		case BON_CTRL_UINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE:
		case BON_CTRL_UINT64_LE:
		case BON_CTRL_UINT64_BE:
		case BON_CTRL_FLOAT32_LE:
		case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:
		case BON_CTRL_FLOAT64_BE:
			return BON_TRUE;
			
		default:
			return BON_FALSE;
	}
}


/////////////////////////////////////

void bon_free_type(bon_type* t) {
	switch (t->id) {
		case BON_TYPE_ARRAY:
			bon_free_type(t->u.array->type);
			free(t->u.array);
			break;
			
		case BON_TYPE_STRUCT: {
			for (bon_size i=0; i<t->u.strct->size; ++i) {
				bon_free_type( t->u.strct->types[i] );
			}
			free(t->u.strct);
		} break;
			
		default:
			break;
	}
	
	free(t);
}

bon_type* bon_new_type_simple(bon_type_id id) {
	bon_type* t = calloc(1, sizeof(bon_type));
	t->id = id;
	return t;
}

bon_type* bon_new_type_simple_array(bon_size n, bon_type_id id) {
	return bon_new_type_array(n, bon_new_type_simple(id));
}

bon_type* bon_new_type_array(bon_size n, bon_type* type) {
	bon_type* t = calloc(1, sizeof(bon_type));
	t->id = BON_TYPE_ARRAY;
	t->u.array = calloc(1, sizeof(bon_type_array));
	t->u.array->size = n;
	t->u.array->type = type;
	return t;
}

bon_type* bon_new_type_struct(bon_size n, const char** names, bon_type** types) {
	
	bon_type* t = calloc(1, sizeof(bon_type));
	t->id = BON_TYPE_STRUCT;
	t->u.strct = calloc(1, sizeof(bon_type_struct));
	t->u.strct->size = n;
	t->u.strct->keys = names; // FIXME: copy?
	t->u.strct->types = types; // FIXME: copy?
	return t;
}


bon_type* bon_new_type_fmt_ap(const char** fmt, va_list* ap);

bon_type* bon_new_type_fmt_ap_obj(const char** fmt, va_list* ap)
{
	if (**fmt != '{') {
		return NULL;
	}
	++*fmt;
	
	const int MAX_OBJ_SIZE = 64; // FIXME: lazy
	
	bon_type_struct* strct = ALLOC_TYPE(1, bon_type_struct);
	strct->size   = 0;
	strct->keys   = ALLOC_TYPE(MAX_OBJ_SIZE, const char*);
	strct->types  = ALLOC_TYPE(MAX_OBJ_SIZE, bon_type*  );
	
	bon_type* ret = ALLOC_TYPE(1, bon_type);
	ret->id = BON_TYPE_STRUCT;
	ret->u.strct = strct;
	
	for (;;) {
		switch (**fmt) {
			case '}':
				++*fmt;
				return ret;
				
			case '\0':
				fprintf(stderr, "obj with no ending }\n");
				bon_free_type(ret);
				return NULL;
				
			default: {
				if (strct->size == MAX_OBJ_SIZE) {
					fprintf(stderr, "obj with too many keys\n");
					bon_free_type(ret);
					return NULL;
				}
				
				const char* key = va_arg(*ap, const char*);
				bon_type* t = bon_new_type_fmt_ap(fmt, ap);
				if (!t) {
					bon_free_type(ret);
					return NULL;
				}
				
				strct->keys[strct->size] = key;
				strct->types[strct->size] = t;
				++strct->size;
			}
		}
	}
}

bon_bool bon_parse_size(const char** fmt, bon_size* out)
{
	if (**fmt < '1' || '9' < **fmt) {
		return BON_FALSE;
	}
	
	bon_size size = (bon_size)(**fmt - '0');
	++*fmt;
	
	while ('0' <= **fmt && **fmt <= '9') {
		size *= 10;
		size += (bon_size)(**fmt - '0');
		++*fmt;
	}
	
	*out = size;
	return BON_TRUE;
}

bon_type* bon_new_type_fmt_ap(const char** fmt, va_list* ap)
{
	switch (**fmt) {
		case '{': {
			return bon_new_type_fmt_ap_obj(fmt, ap);
		}
			
		case '[': {
			++*fmt;
			
			bon_size n;
			if (!bon_parse_size(fmt, &n)) {
				return NULL;
			}
			bon_type* t = bon_new_type_fmt_ap(fmt, ap);
			if (!t) {
				return NULL;
			}
			if (**fmt != ']') {
				fprintf(stderr, "malformed array\n");
				bon_free_type(t);
				return NULL;
			}
			++*fmt;
			return bon_new_type_array(n, t);
		}
			
		case 'f': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_FLOAT32);
		}
			
		case 'd': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_FLOAT32);
		}
			
		case 's':
		case 'u':
		{
			bon_bool sgnd = (**fmt == 's');
			++*fmt;
			
			bon_size n;
			if (!bon_parse_size(fmt, &n)) {
				return NULL;
			}
			switch (n) {
				case 8:   return bon_new_type_simple(sgnd ? BON_TYPE_SINT8  : BON_TYPE_UINT8 );
				case 16:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT16 : BON_TYPE_UINT16);
				case 32:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT32 : BON_TYPE_UINT32);
				case 64:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT64 : BON_TYPE_UINT64);
				default:
					fprintf(stderr, "Bad integer size: %d\n", (int)n);
					return NULL;
			}
		}
			
		default:
			fprintf(stderr, "Unknown format\n");
			return NULL;
	}
}

bon_type* bon_new_type_fmt(const char* fmt, ...)
{
	bon_type* ret;
	
	va_list ap;
	
	va_start(ap, fmt);
	
	ret = bon_new_type_fmt_ap(&fmt, &ap);
	
	va_end(ap);
	
	return ret;
}

/* the size of the payload. */
bon_size bon_aggregate_payload_size(const bon_type* type)
{
	switch (type->id) {
		case BON_TYPE_ARRAY:
			return type->u.array->size *
			bon_aggregate_payload_size(type->u.array->type);
			
		case BON_TYPE_STRUCT: {
			bon_size sum = 0;
			bon_type_struct* strct = type->u.strct;
			for (bon_size i=0; i<strct->size; ++i) {
				sum += bon_aggregate_payload_size(strct->types[i]);
			}
			return sum;
		}
			
		default:
			return bon_w_type_size( type->id );
	}
}

/////////////////////////////////////

bon_bool bon_vec_writer(void* userData, const void* data, uint64_t nbytes) {
	bon_byte_vec* vec = (bon_byte_vec*)userData;
	bon_size oldSize = vec->size;
	VECTOR_EXPAND(*vec, uint8_t, nbytes);
	if (vec->data) {
		memcpy(vec->data + oldSize, data, nbytes);
		return BON_TRUE;
	} else {
		return BON_FALSE;
	}
}
	
	
bon_bool bon_file_writer(void* user, const void* data, uint64_t nbytes)
{
	FILE* fp = (FILE*)user;
	fwrite(data, 1, nbytes, fp);
	return !ferror(fp);
}


/////////////////////////////////////

uint16_t swap_endian_uint16(uint16_t us)
{
	return (uint16_t)( (us >> 8) | (us << 8) );
}

uint32_t swap_endian_uint32(uint32_t ui)
{
	ui = (ui >> 24) |
	((ui<<8) & 0x00FF0000) |
	((ui>>8) & 0x0000FF00) |
	(ui << 24);
	return ui;
}

uint64_t swap_endian_uint64(uint64_t ull)
{
	ull = (ull >> 56) |
	((ull<<40) & 0x00FF000000000000ULL) |
	((ull<<24) & 0x0000FF0000000000ULL) |
	((ull<<8 ) & 0x000000FF00000000ULL) |
	((ull>>8 ) & 0x00000000FF000000ULL) |
	((ull>>24) & 0x0000000000FF0000ULL) |
	((ull>>40) & 0x000000000000FF00ULL) |
	(ull << 56);
	return ull;
}

#if __LITTLE_ENDIAN__

uint16_t uint16_to_le(uint16_t v) {
	return v;
}

uint32_t uint32_to_le(uint32_t v) {
	return v;
}

uint64_t uint64_to_le(uint64_t v) {
	return v;
}


uint16_t le_to_uint16(uint16_t v) {
	return v;
}

uint32_t le_to_uint32(uint32_t v) {
	return v;
}

uint64_t le_to_uint64(uint64_t v) {
	return v;
}

uint16_t be_to_uint16(uint16_t v) {
	return swap_endian_uint16( v );
}

uint32_t be_to_uint32(uint32_t v) {
	return swap_endian_uint32( v );
}

uint64_t be_to_uint64(uint64_t v) {
	return swap_endian_uint64( v );
}

#else

uint16_t uint16_to_le(uint16_t v) {
	return swap_endian_uint16( v );
}

uint32_t uint32_to_le(uint32_t v) {
	return swap_endian_uint32( v );
}

uint64_t uint64_to_le(uint64_t v) {
	return swap_endian_uint64( v );
}


uint16_t le_to_uint16(uint16_t v) {
	return swap_endian_uint16( v );
}

uint32_t le_to_uint32(uint32_t v) {
	return swap_endian_uint32( v );
}

uint64_t le_to_uint64(uint64_t v) {
	return swap_endian_uint64( v );
}

uint16_t be_to_uint16(uint16_t v) {
	return v;
}

uint32_t be_to_uint32(uint32_t v) {
	return v;
}

uint64_t be_to_uint64(uint64_t v) {
	return v;
}

#endif


/////////////////////////////////////


const char* bon_err_str(bon_error err)
{
	if (BON_NUM_ERR <= err)
		return "BON_UNKNOWN_ERROR";
	
	const char* err_str[BON_NUM_ERR] = {
		"BON_SUCCESS",
		
		"BON_ERR_WRITE_ERROR",
		"BON_ERR_BAD_AGGREGATE_SIZE",
		
		"BON_ERR_TOO_SHORT",
		"BON_ERR_BAD_HEADER",
		"BON_ERR_BAD_VLQ",
		"BON_ERR_MISSING_LIST_END",
		"BON_ERR_MISSING_OBJ_END",
		"BON_ERR_BAD_CTRL",
		"BON_ERR_KEY_NOT_STRING",
		"BON_ERR_BAD_AGGREGATE_TYPE",
		"BON_ERR_BAD_TYPE",
		"BON_ERR_STRING_NOT_ZERO_ENDED",
		"BON_ERR_MISSING_TOKEN",
		
		"BON_ERR_TRAILING_DATA",
		"BON_ERR_BAD_BLOCK",
		
		"BON_ERR_NARROWING",
		"BON_ERR_NULL_OBJ",
	};
	
	return err_str[err];
}

/////////////////////////////////////


// Helper for reading without overflowing:

void br_set_err(bon_reader* br, bon_error err)
{
	fprintf(stderr, "Setting error %s\n", bon_err_str(err));
	if (br->error == BON_SUCCESS) {
		br->error = err;
	}
}

void br_assert(bon_reader* br, bon_bool statement, bon_error onFail)
{
	if (!statement) {
		br_set_err(br, onFail);
	}
}

void br_skip(bon_reader* br, size_t n) {
	assert(br->nbytes >= n);
	br->data   += n;
	br->nbytes -= n;
}

// next byte [0,255] or -1
int peek(bon_reader* br) {
	if (br->nbytes > 0) {
		return br->data[0];
	} else {
		return -1;
	}
}

uint8_t next(bon_reader* br) {
	if (br->nbytes > 0) {
		uint8_t ret = br->data[0];
		br->data   += 1;
		br->nbytes -= 1;
		return ret;
	} else {
		br_set_err(br, BON_ERR_TOO_SHORT);
		return 0;
	}
}

void br_putback(bon_reader* br) {
	if (!br->error) {
		br->data--;
		br->nbytes++;
	}
}

bon_bool read(bon_reader* br, uint8_t* out, size_t n) {
	if (br->nbytes >= n) {
		memcpy(out, br->data, n);
		br->data   += n;
		br->nbytes -= n;
		return BON_TRUE;
	} else {
		br_set_err(br, BON_ERR_TOO_SHORT);
		return BON_FALSE;
	}
}

// Returns a pointer to the next 'size' bytes, or NULL if fail
const uint8_t* br_read(bon_reader* br, size_t size)
{
	const uint8_t* ptr = br->data;
	br_skip(br, size);
	if (br->error) return 0;
	return ptr;
}

// Will swallow a token, or set error if next byte isn't this token.
void br_swallow(bon_reader* br, uint8_t token) {
	if (next(br) != token) {
		br_set_err(br, BON_ERR_MISSING_TOKEN);
	}
}

/////////////////////////////////////

// Helper for writing to a binary stream without overflowing:

typedef struct {
	uint8_t*        data;
	bon_size        nbytes;
	bon_error  error;
} bon_writer;

void bw_set_err(bon_writer* bw, bon_error err)
{
	fprintf(stderr, "Setting error %s\n", bon_err_str(err));
	if (bw->error == BON_SUCCESS) {
		bw->error = err;
	}
}

void bw_assert(bon_writer* bw, bon_bool statement, bon_error onFail)
{
	if (!statement) {
		bw_set_err(bw, onFail);
	}
}


bon_bool bw_skip(bon_writer* bw, size_t n) {
	if (bw->nbytes >= n) {
		bw->data   += n;
		bw->nbytes -= n;
		return BON_TRUE;
	} else {
		bw_set_err(bw, BON_ERR_TOO_SHORT);
		return BON_FALSE;
	}
}

bon_bool bw_write_raw(bon_writer* bw, const void* in, size_t n) {
	if (bw->nbytes >= n) {
		memcpy(bw->data, in, n);
		bw->data   += n;
		bw->nbytes -= n;
		return BON_TRUE;
	} else {
		bw_set_err(bw, BON_ERR_TOO_SHORT);
		return BON_FALSE;
	}
}

bon_bool bw_write_uint8(bon_writer* bw, uint8_t val) {
	return bw_write_raw(bw, &val, 1);
}

bon_bool bw_write_uint16(bon_writer* bw, uint16_t val) {
	return bw_write_raw(bw, &val, sizeof(uint16_t));
}

bon_bool bw_write_uint32(bon_writer* bw, uint32_t val) {
	return bw_write_raw(bw, &val, sizeof(uint32_t));
}

bon_bool bw_write_uint64(bon_writer* bw, uint64_t val) {
	return bw_write_raw(bw, &val, sizeof(uint64_t));
}

// With checks for narrowing
bon_bool bw_write_uint_as_uint8(bon_writer* bw, uint64_t val) {
	if (val <= 0xff) {
		return bw_write_uint8(bw, (uint8_t)val);
	} else {
		bw_set_err(bw, BON_ERR_NARROWING);
		return BON_FALSE;
	}
}

/////////////////////////////////////


bon_error bon_w_get_error(bon_w_doc* doc) {
	return doc->error;
}

void bon_w_set_error(bon_w_doc* doc, bon_error err)
{
	fprintf(stderr, "Setting error %s\n", bon_err_str(err));
	if (doc->error == BON_SUCCESS) {
		doc->error = err;
	}
}


void bon_w_assert(bon_w_doc* doc, bon_bool statement, bon_error onFail)
{
	if (!statement) {
		bon_w_set_error(doc, onFail);
	}
}


/////////////////////////////////////
// Writing
	
void bon_w_flush(bon_w_doc* doc) {
	if (doc->buff_ix > 0) {
		if (!doc->writer(doc->userData, doc->buff, doc->buff_ix)) {
			doc->error = BON_ERR_WRITE_ERROR;
		}
		doc->buff_ix = 0;
	}
}

void bon_w_raw(bon_w_doc* doc, const void* data, bon_size bs) {
	if (doc->buff) {
		if (doc->buff_ix + bs < doc->buff_size) {
			memcpy(doc->buff + doc->buff_ix, data, bs);
			doc->buff_ix += bs;
		} else {
			bon_w_flush(doc);
			
			if (bs < doc->buff_size) {
				memcpy(doc->buff, data, bs);
				doc->buff_ix = bs;
			} else {
				// Too large for buffer - write directly:
				if (!doc->writer(doc->userData, data, bs)) {
					doc->error = BON_ERR_WRITE_ERROR;
				}
			}
		}
	} else {
		// Unbuffered
		if (!doc->writer(doc->userData, data, bs)) {
			doc->error = BON_ERR_WRITE_ERROR;
		}
	}
}

void bon_w_raw_uint8(bon_w_doc* doc, uint8_t val) {
	bon_w_raw(doc, &val, sizeof(val));
}

void bon_w_raw_uint16(bon_w_doc* doc, uint16_t val) {
	bon_w_raw(doc, &val, sizeof(val));
}

void bon_w_raw_uint32(bon_w_doc* doc, uint32_t val) {
	bon_w_raw(doc, &val, sizeof(val));
}

void bon_w_raw_uint64(bon_w_doc* doc, uint64_t val) {
	bon_w_raw(doc, &val, sizeof(val));
}


/////////////////////////////////////
// VarInt - standard VLQ
// See http://en.wikipedia.org/wiki/Variable-length_quantity
// See http://rosettacode.org/wiki/Variable-length_quantity

// Maximum numbero f bytes to encode a very large number
#define BON_VARINT_MAX_LEN 10

uint32_t bon_w_vlq_size(bon_size x)
{
	if (x < (1ULL <<  7))  return  1;
	if (x < (1ULL << 14))  return  2;
	if (x < (1ULL << 21))  return  3;
	if (x < (1ULL << 28))  return  4;
	if (x < (1ULL << 35))  return  5;
	if (x < (1ULL << 42))  return  6;
	if (x < (1ULL << 49))  return  7;
	if (x < (1ULL << 56))  return  8;
	if (x < (1ULL << 63))  return  9;
	                       return 10;
}

// Returns number of bytes written
uint32_t bon_w_vlq_to(uint8_t* out, bon_size x)
{
	uint32_t size = bon_w_vlq_size(x);
	
	for (uint32_t i = 0; i < size; ++i) {
		out[i] = ((x >> ((size - 1 - i) * 7)) & 0x7f) | 0x80;
	}
	
	out[size-1] &= 0x7f; // Remove last flag
	
	return size;
}

uint32_t bon_w_vlq(bon_w_doc* doc, bon_size x)
{
	uint8_t vals[BON_VARINT_MAX_LEN];
	uint32_t size = bon_w_vlq_to(vals, x);
	bon_w_raw(doc, vals, size);
	return size;
}

uint64_t br_read_vlq(bon_reader* br)
{
	uint64_t r = 0;
	uint32_t size = 0; // Sanity check
	
	for (;;) {
		uint8_t in = next(br);
		++size;
		r = (r << 7) | (uint64_t)(in & 0x7f);
		
		if ((in & 0x80) == 0)
			break;
		
		if (size == BON_VARINT_MAX_LEN) {
			br_set_err(br, BON_ERR_BAD_VLQ);
			return 0;
		}
	};
	
	return r;
}

/////////////////////////////////////


bon_w_doc* bon_w_new_doc(bon_w_writer_t writer, void* userData)
{
	bon_w_doc* doc = (bon_w_doc*)malloc(sizeof(bon_w_doc));
	memset(doc, 0, sizeof(bon_w_doc));
	doc->writer    = writer;
	doc->userData  = userData;
	doc->error     = BON_SUCCESS;
	
	const unsigned BUFF_SIZE = 64*1024;
	doc->buff      = malloc(BUFF_SIZE);
	doc->buff_ix   = 0;
	doc->buff_size = BUFF_SIZE;
	
	return doc;
}

void bon_w_close_doc(bon_w_doc* doc)
{
	bon_w_flush(doc);
	free(doc->buff);
	free(doc);
}

void bon_w_header(bon_w_doc* doc)
{
	const uint8_t header[4] = {'B', 'O', 'N', '0'};
	bon_w_raw(doc, header, 4);
}

void bon_w_footer(bon_w_doc* doc)
{
}

//////////////////////////////////////
// Value writing

void bon_w_begin_obj(bon_w_doc* doc) {
	bon_w_raw_uint8(doc, BON_CTRL_OBJ_BEGIN);
}

void bon_w_end_obj(bon_w_doc* doc) {
	bon_w_raw_uint8(doc, BON_CTRL_OBJ_END);
}

void bon_w_key(bon_w_doc* doc, const char* utf8, bon_size nbytes) {
	bon_w_string(doc, utf8, nbytes);
}

void bon_w_begin_list(bon_w_doc* doc) {
	bon_w_raw_uint8(doc, BON_CTRL_LIST_BEGIN);
}

void bon_w_end_list(bon_w_doc* doc) {
	bon_w_raw_uint8(doc, BON_CTRL_LIST_END);
}

void bon_w_nil(bon_w_doc* doc) {
	bon_w_raw_uint8(doc, BON_CTRL_NIL);
}

void bon_w_bool(bon_w_doc* doc, bon_bool val) {
	if (val) {
		bon_w_raw_uint8(doc, BON_CTRL_TRUE);
	} else {
		bon_w_raw_uint8(doc, BON_CTRL_FALSE);
	}
}

void bon_w_string(bon_w_doc* doc, const char* utf8, bon_size nbytes) {
	if (nbytes==BON_ZERO_ENDED) {
		nbytes = strlen(utf8);
	}
	
	// TODO: small string optimization
	
	bon_w_raw_uint8(doc, BON_CTRL_STRING_VLQ);
	bon_w_vlq(doc, nbytes);
	bon_w_raw(doc, utf8, nbytes);
	bon_w_raw_uint8(doc, 0); // Zero-ended
}

void bon_w_uint64(bon_w_doc* doc, uint64_t val)
{
	if (val == (val&0xff)) {
		bon_w_raw_uint8(doc, BON_CTRL_UINT8);
		bon_w_raw_uint8(doc, (uint8_t)val);
	} else if (val == (val&0xffff)) {
		bon_w_raw_uint8(doc, BON_CTRL_UINT16);
		bon_w_raw_uint16(doc, (uint16_t)val);
	} else if (val == (val&0xffffffff)) {
		bon_w_raw_uint8(doc, BON_CTRL_UINT32);
		bon_w_raw_uint32(doc, (uint32_t)val);
	} else {
		bon_w_raw_uint8(doc, BON_CTRL_UINT64);
		bon_w_raw_uint64(doc, (uint64_t)val);
	}
}

void bon_w_sint64(bon_w_doc* doc, int64_t val) {
	if (val >= 0) {
		bon_w_uint64(doc, (uint64_t)val);
	} else if (-0x80 <= val && val < 0x80) {
		bon_w_raw_uint8(doc, BON_CTRL_SINT8);
		bon_w_raw_uint8(doc, (uint8_t)val);
	} else if (-0x8000 <= val && val < 0x8000) {
		bon_w_raw_uint8(doc, BON_CTRL_SINT16);
		bon_w_raw_uint16(doc, (uint16_t)val);
	} else if (-0x80000000LL <= val && val < 0x80000000LL) {
		bon_w_raw_uint8(doc, BON_CTRL_SINT32);
		bon_w_raw_uint32(doc, (uint32_t)val);
	} else {
		bon_w_raw_uint8(doc, BON_CTRL_SINT64);
		bon_w_raw_uint64(doc, (uint64_t)val);
	}
}

void bon_w_float(bon_w_doc* doc, float val)
{
	bon_w_raw_uint8(doc, BON_CTRL_FLOAT32);
	bon_w_raw(doc, &val, 4);
}

void bon_w_double(bon_w_doc* doc, double val)
{
	if (!isfinite(val) || (double)(float)val == val) {
		bon_w_float(doc, (float)val);
	} else {
		bon_w_raw_uint8(doc, BON_CTRL_FLOAT64);
		bon_w_raw(doc, &val, 8);
	}
}

void bon_w_aggregate_type(bon_w_doc* doc, bon_type* type);

void bon_w_array_type(bon_w_doc* doc, bon_size length, bon_type* element_type)
{
	// TODO: compressed array
	
	bon_w_raw_uint8(doc, BON_CTRL_ARRAY_VLQ);
	bon_w_vlq(doc, length);
	bon_w_aggregate_type(doc, element_type);
}

void bon_w_aggregate_type(bon_w_doc* doc, bon_type* type)
{
	switch (type->id) {
		case BON_TYPE_ARRAY:
			bon_w_array_type(doc, type->u.array->size, type->u.array->type);
			break;
			
			
		case BON_TYPE_STRUCT: {
			// TODO: compressed struct
			
			bon_w_raw_uint8(doc, BON_CTRL_STRUCT_VLQ);
			bon_type_struct* strct = type->u.strct;
			bon_size n = strct->size;
			bon_w_vlq(doc, n);
			for (bon_size i=0; i<n; ++i) {
				bon_w_string(doc, strct->keys[i], BON_ZERO_ENDED);
				bon_w_aggregate_type(doc, strct->types[i]);
			}
		} break;
			
			
		default:
			// Simple type - write ctrl code:
			bon_w_raw_uint8(doc, type->id);
			break;
	}
}

void bon_w_aggregate(bon_w_doc* doc, bon_type* type, const void* data, bon_size nbytes)
{
	// Sanity check:
	bon_size expected_size = bon_aggregate_payload_size(type);
	if (nbytes != expected_size) {
		fprintf(stderr, "bon_w_aggregate: wrong size. Expected %d, got %d\n", (int)expected_size, (int)nbytes);
		bon_w_set_error(doc, BON_ERR_BAD_AGGREGATE_SIZE);
		return;
	}
	
	bon_w_aggregate_type(doc, type);
	bon_w_raw(doc, data, nbytes);
}

void bon_w_array(bon_w_doc* doc, bon_size len, bon_type_id element_t,
					  const void* data, bon_size nbytes)
{
	bon_w_assert(doc, len * bon_w_type_size(element_t) == nbytes,
					 BON_ERR_BAD_AGGREGATE_SIZE);
	if (doc->error) {
		return;
	}
	
	// TODO: compressed arrays
	
	bon_w_raw_uint8(doc, BON_CTRL_ARRAY_VLQ);
	bon_w_vlq(doc, len);
	bon_w_raw_uint8(doc, element_t);
	
	bon_w_raw(doc, data, nbytes);
}


////////////////////////////////////////////
// BON bon_reader API


////////////////////////////////////////////////////////

void bon_r_list_values(bon_reader* br, bon_values* vals);
void bon_r_kvs(bon_reader* br, bon_kvs* kvs);

uint16_t br_read_u16(bon_reader* br) {
	const uint8_t* ptr = br->data;
	br_skip(br, 2);
	if (br->error) {
		return 0;
	} else {
		return *(const uint16_t*)ptr;
	}
}

uint32_t br_read_u32(bon_reader* br) {
	const uint8_t* ptr = br->data;
	br_skip(br, 4);
	if (br->error) {
		return 0;
	} else {
		return *(const uint32_t*)ptr;
	}
}

uint64_t br_read_u64(bon_reader* br) {
	const uint8_t* ptr = br->data;
	br_skip(br, 8);
	if (br->error) {
		return 0;
	} else {
		return *(const uint64_t*)ptr;
	}
}


float br_read_float_native(bon_reader* br) {
	const uint8_t* ptr = br_read(br, sizeof(float));
	if (ptr) {
		return *(const float*)ptr;
	} else {
		return 0;
	}
}

float br_read_float_swapped(bon_reader* br) {
	const uint8_t* ptr = br_read(br, sizeof(float));
	if (ptr) {
		const uint8_t data[4] = { ptr[3], ptr[2], ptr[1], ptr[0] };
		return *(const float*)data;
	} else {
		return 0;
	}
}


double br_read_double_native(bon_reader* br) {
	const uint8_t* ptr = br_read(br, sizeof(double));
	if (ptr) {
		return *(const double*)ptr;
	} else {
		return 0;
	}
}

double br_read_double_swapped(bon_reader* br) {
	const uint8_t* ptr = br_read(br, sizeof(double));
	if (ptr) {
		const uint8_t data[8] = { ptr[7], ptr[6], ptr[5], ptr[4], ptr[3], ptr[2], ptr[1], ptr[0] };
		return *(const double*)data;
	} else {
		return 0;
	}
}


double br_read_double(bon_reader* br, bon_type_id t) {
	switch (t) {
		case BON_CTRL_FLOAT32_LE:
#if __LITTLE_ENDIAN__
			return br_read_float_native(br);
#else
			return br_read_float_swapped(br);
#endif
			
		case BON_CTRL_FLOAT32_BE:
#if __LITTLE_ENDIAN__
			return br_read_float_swapped(br);
#else
			return br_read_float_native(br);
#endif
			
			
		case BON_CTRL_FLOAT64_LE:
#if __LITTLE_ENDIAN__
			return br_read_double_native(br);
#else
			return br_read_double_swapped(br);
#endif
			
			
		case BON_CTRL_FLOAT64_BE:
#if __LITTLE_ENDIAN__
			return br_read_double_swapped(br);
#else
			return br_read_double_native(br);
#endif
			
		default:
			br_set_err(br, BON_ERR_BAD_TYPE);
			return 0;
	}
}

int64_t br_read_sint64(bon_reader* br, bon_type_id t) {
	switch (t) {
		case BON_TYPE_SINT8:      return (int8_t)next(br);
		case BON_TYPE_SINT16_LE:  return (int16_t)le_to_uint16( br_read_u16(br) );
		case BON_TYPE_SINT16_BE:  return (int16_t)be_to_uint16( br_read_u16(br) );
		case BON_TYPE_SINT32_LE:  return (int32_t)le_to_uint32( br_read_u32(br) );
		case BON_TYPE_SINT32_BE:  return (int32_t)be_to_uint32( br_read_u32(br) );
		case BON_TYPE_SINT64_LE:  return (int64_t)le_to_uint64( br_read_u64(br) );
		case BON_TYPE_SINT64_BE:  return (int64_t)be_to_uint64( br_read_u64(br) );
			
		default:
			br_set_err(br, BON_ERR_BAD_TYPE);
			return 0;
	}
}

uint64_t br_read_uint64(bon_reader* br, bon_type_id t) {
	switch (t) {
		case BON_TYPE_UINT8:      return (uint8_t)next(br);
		case BON_TYPE_UINT16_LE:  return (uint16_t)le_to_uint16( br_read_u16(br) );
		case BON_TYPE_UINT16_BE:  return (uint16_t)be_to_uint16( br_read_u16(br) );
		case BON_TYPE_UINT32_LE:  return (uint32_t)le_to_uint32( br_read_u32(br) );
		case BON_TYPE_UINT32_BE:  return (uint32_t)be_to_uint32( br_read_u32(br) );
		case BON_TYPE_UINT64_LE:  return (uint64_t)le_to_uint64( br_read_u64(br) );
		case BON_TYPE_UINT64_BE:  return (uint64_t)be_to_uint64( br_read_u64(br) );
			
		default:
			br_set_err(br, BON_ERR_BAD_TYPE);
			return 0;
	}
}

void bon_r_value(bon_reader* br, bon_value* val);
void parse_aggr_type(bon_reader* br, bon_type* type);

void parse_array_type(bon_reader* br, bon_type* type, bon_size arraySize)
{
	type->id = BON_TYPE_ARRAY;
	bon_type_array* array = ALLOC_TYPE(1, bon_type_array);
	type->u.array = array;
	array->size = arraySize;
	array->type = (bon_type*)calloc(1, sizeof(bon_type));
	parse_aggr_type(br, array->type);
}

void parse_struct_type(bon_reader* br, bon_type* type, bon_size structSize)
{
	type->id = BON_TYPE_STRUCT;
	bon_type_struct* strct = ALLOC_TYPE(1, bon_type_struct);
	type->u.strct = strct;
	strct->size   = structSize;
	strct->keys   = ALLOC_TYPE(strct->size, const char*);
	strct->types  = ALLOC_TYPE(strct->size, bon_type*);
	
	for (bon_size ti=0; ti<strct->size; ++ti) {
		bon_value key;
		bon_r_value(br, &key);
		if (key.type != BON_VALUE_STRING) {
			br_set_err(br, BON_ERR_KEY_NOT_STRING);
			return;
		}
		
		strct->keys[ti] = (const char*)key.u.str.ptr;
		strct->types[ti] = ALLOC_TYPE(1, bon_type);
		parse_aggr_type(br, strct->types[ti]);
	}
}

void parse_aggr_type(bon_reader* br, bon_type* type)
{
	memset(type, 0, sizeof(bon_type));
	
	if (br->error)
		return;
	
	uint8_t ctrl = next(br);
	
	if (FIXED_AGGREGATES_START <= ctrl   &&  ctrl < FIXED_NEG_INT_START)
	{
		// Compressed array or struct:
		if (ctrl  >=  FIXED_STRUCT_START)
		{
			bon_size structSize = ctrl - FIXED_STRUCT_START;
			parse_struct_type(br, type, structSize);
		}
		else if (ctrl  >=  FIXED_BYTE_ARRAY_START)
		{
			bon_size arraySize     = ctrl - FIXED_BYTE_ARRAY_START;
			
			bon_type_array* array  = ALLOC_TYPE(1, bon_type_array);
			array->size            = arraySize;
			array->type            = (bon_type*)calloc(1, sizeof(bon_type));
			array->type->id        = BON_TYPE_UINT8;
			
			type->id               = BON_TYPE_ARRAY;
			type->u.array          = array;
		}
		else
		{
			assert(ctrl >= FIXED_ARRAY_START);
			bon_size arraySize = ctrl - FIXED_ARRAY_START;
			parse_array_type(br, type, arraySize);
		}
	}
	else if (ctrl == BON_CTRL_ARRAY_VLQ) {
		parse_array_type(br, type, br_read_vlq(br));
	} else if (ctrl == BON_CTRL_STRUCT_VLQ) {
		parse_struct_type(br, type, br_read_vlq(br));
	} else if (bon_is_simple_type(ctrl)) {
		// float, int - recursion done
		type->id = ctrl;
	} else {
		// not array, not tuple, not atomic type? Error!
		br_set_err(br, BON_ERR_BAD_AGGREGATE_TYPE);
	}
}

void bon_r_aggr_value(bon_reader* br, bon_value* val)
{
	val->type = BON_VALUE_AGGREGATE;
	bon_type* type = &val->u.agg.type;
	parse_aggr_type(br, type);
	val->u.agg.data = br->data;
	bon_size nBytesPayload = bon_aggregate_payload_size(type);
	br_skip(br, nBytesPayload);
	
	if (br->doc) {
		br->doc->stats.count_aggr      +=  1;
		br->doc->stats.bytes_aggr_dry  +=  nBytesPayload;
		//br->doc->stats.bytes_aggr_wet  += (bytes_left_start - br->nbytes);
	}
}

void bon_r_string_sized(bon_reader* br, bon_value* val, size_t strLen)
{
	val->type       = BON_VALUE_STRING;
	val->u.str.size = strLen;
	val->u.str.ptr  = br->data;
	br_skip(br, strLen);
	int zero = next(br);
	
	if (zero != 0) {
		br_set_err(br, BON_ERR_STRING_NOT_ZERO_ENDED);
	}
	
	if (br->doc) {
		br->doc->stats.count_string      += 1;
		br->doc->stats.bytes_string_dry  += val->u.str.size;
	}
}

// We've read a control byte - read the value following it.
void bon_r_value_from_ctrl(bon_reader* br, bon_value* val, uint8_t ctrl)
{
	switch (ctrl)
	{
		case BON_CTRL_BLOCK_REF:
			val->type = BON_VALUE_BLOCK_REF;
			val->u.blockRefId = br_read_vlq(br);
			
			
		case BON_CTRL_STRING_VLQ: {
			bon_size strLen = br_read_vlq(br);
			bon_r_string_sized(br, val, strLen);
		} break;
			
			
		case BON_CTRL_TRUE:
			val->type      = BON_VALUE_BOOL;
			val->u.boolean = BON_TRUE;
			break;
			
			
		case BON_CTRL_FALSE:
			val->type      = BON_VALUE_BOOL;
			val->u.boolean = BON_FALSE;
			break;
			
			
		case BON_CTRL_NIL:
			val->type = BON_VALUE_NIL;
			break;
			
			
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE:
			val->type   = BON_VALUE_SINT64;
			val->u.s64  = br_read_sint64(br, ctrl);
		break;
			
			
		case BON_CTRL_UINT8:
		case BON_CTRL_UINT16_LE:
		case BON_CTRL_UINT16_BE:
		case BON_CTRL_UINT32_LE:
		case BON_CTRL_UINT32_BE:
		case BON_CTRL_UINT64_LE:
		case BON_CTRL_UINT64_BE:
			val->type   = BON_VALUE_UINT64;
			val->u.u64  = br_read_uint64(br, ctrl);
		break;			
			
			
		case BON_CTRL_FLOAT32_LE:
		case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:
		case BON_CTRL_FLOAT64_BE:
			val->type  = BON_VALUE_DOUBLE;
			val->u.dbl = br_read_double(br, ctrl);
		break;
			
			
		case BON_CTRL_LIST_BEGIN:
			val->type          = BON_VALUE_LIST;
			bon_r_list_values(br, &val->u.list.values);
			br_swallow(br, BON_CTRL_LIST_END);
			break;
			
			
		case BON_CTRL_OBJ_BEGIN:
			val->type      = BON_VALUE_OBJ;
			bon_r_kvs(br, &val->u.obj.kvs);
			if (next(br) != BON_CTRL_OBJ_END) {
				br_set_err(br, BON_ERR_MISSING_OBJ_END);
			}
			break;
			
			
		case BON_CTRL_ARRAY_VLQ:
		case BON_CTRL_TUPLE_VLQ:
		case BON_CTRL_STRUCT_VLQ: {
			br_putback(br);
			bon_r_aggr_value(br, val);
		} break;
			
			
		default: {
			br_set_err(br, BON_ERR_BAD_CTRL);
		}
	}
}


void bon_r_value(bon_reader* br, bon_value* val)
{
	uint8_t ctrl = next(br);
	
	
	if     (ctrl  >=  FIXED_NEG_INT_START)
	{
		val->type   = BON_VALUE_SINT64;
		val->u.s64  = (int8_t)ctrl;
	}
	else if (ctrl >= FIXED_AGGREGATES_START)
	{
		br_putback(br);
		bon_r_aggr_value(br, val);
	}
	else if (ctrl  >=  FIXED_BLOCK_START)
	{
		val->type          = BON_VALUE_BLOCK_REF;
		val->u.blockRefId  = ctrl - FIXED_BLOCK_START;
	}
	else if (ctrl  >=  FIXED_CODES_START)
	{
		bon_r_value_from_ctrl(br, val, ctrl);
	}
	else if (ctrl  >=  FIXED_STRING_START)
	{
		// FixString
		bon_r_string_sized(br, val, ctrl - FIXED_STRING_START);
	}
	else
	{
		// PosFixNum
		assert(ctrl < 32);
		val->type   = BON_VALUE_UINT64;
		val->u.u64  = (uint64_t)ctrl;
	}
}


void bon_r_list_values(bon_reader* br, bon_values* vals)
{
	memset(vals, 0, sizeof(bon_values));
	
	while (!br->error)
	{
		if (peek(br) == BON_CTRL_LIST_END) {
			break;
		}
		
		VECTOR_EXPAND(*vals, bon_value, 1)
		bon_r_value(br, vals->data + vals->size - 1);
	}
}


void bon_r_kvs(bon_reader* br, bon_kvs* kvs)
{
	memset(kvs, 0, sizeof(bon_kvs));
	
	while (!br->error)
	{
		int ctrl = peek(br);
		if (ctrl == BON_CTRL_OBJ_END) {
			break;
		}
		
		VECTOR_EXPAND(*kvs, bon_kv, 1)
		
		bon_kv* kv = kvs->data + kvs->size - 1;
		bon_r_value(br, &kv->key);
		br_assert(br, kv->key.type==BON_VALUE_STRING, BON_ERR_KEY_NOT_STRING);
		bon_r_value(br, &kv->val);
	}
}

void bon_r_header(bon_r_doc* doc, bon_reader* br)
{
	if (peek(br) == BON_CTRL_HEADER)
	{
		uint8_t top[4];
		if (read(br, top, 4) == BON_FALSE)
			return;
		
		const uint8_t header[4] = {'B', 'O', 'N', '0'};
		
		if (memcmp(header, top, 4) != 0) {
			br_set_err(br, BON_ERR_BAD_HEADER);
			return;
		}
	}
}

void bon_r_read_content(bon_r_doc* doc, bon_reader* br)
{
	bon_r_blocks* blocks = &doc->blocks;
		
	if (peek(br) == BON_CTRL_BLOCK_BEGIN)
	{
		// Blocked document
				
		while (!br->error && peek(br)==BON_CTRL_BLOCK_BEGIN) {
			VECTOR_EXPAND(*blocks, bon_r_block, 1);
			bon_r_block* block = &blocks->data[blocks->size-1];
			
			br_swallow(br, BON_CTRL_BLOCK_BEGIN);
			block->id            = br_read_vlq(br);
			block->payload_size  = br_read_vlq(br);
			
			if (block->payload_size >= br->nbytes) {
				br_set_err(br, BON_ERR_BAD_BLOCK);
				return;
			}
						
			block->payload         = br->data;
			
			if (block->payload_size == 0) {
				// Unspecified size - forced parse:
				bon_r_value(br, &block->value);
				block->parsed = BON_TRUE;
			} else {
				// postpone parsing block
				br_skip(br, block->payload_size);
				block->parsed = BON_FALSE;
			}
			
			br_swallow(br, BON_CTRL_BLOCK_END);
		}
	}
	else
	{
		// Block-less document
		blocks->size  = 1;
		blocks->cap   = 1;
		blocks->data  = ALLOC_TYPE(1, bon_r_block);
		bon_r_block* root = blocks->data;
		root->id              = 0;
		root->payload         = br->data;
		root->payload_size    = 0;
		root->parsed          = BON_TRUE;
		bon_r_value(br, &root->value);
	}
}

void bon_r_footer(bon_r_doc* doc, bon_reader* br)
{
	br_assert(br, br->nbytes==0, BON_ERR_TRAILING_DATA);
}

void bon_r_read_doc(bon_r_doc* doc, bon_reader* br)
{
	doc->stats.bytes_file = br->nbytes;
	bon_r_header(doc, br);
	if (br->error) return;
	bon_r_read_content(doc, br);
	if (br->error) return;
	bon_r_footer(doc, br);
}
	
void bon_r_set_error(bon_r_doc* doc, bon_error err)
{
	fprintf(stderr, "Setting error %s\n", bon_err_str(err));
	if (doc->error == BON_SUCCESS) {
		doc->error = err;
	}
}

bon_r_doc* bon_r_open(const uint8_t* data, bon_size nbytes)
{
	bon_r_doc* doc = ALLOC_TYPE(1, bon_r_doc);
	
	bon_reader br_v = { data, nbytes, BON_FALSE, doc };
	bon_reader* br = &br_v;
	
	bon_r_read_doc(doc, br);
	
	doc->error = br->error;
	return doc;
}

void bon_r_close(bon_r_doc* doc)
{
	// TODO: right
	free(doc);
}

// Returns NULL on fail
const bon_value* bon_r_get_block(bon_r_doc* doc, uint64_t id)
{
	for (bon_size bi=0; bi<doc->blocks.size; ++bi) {
		bon_r_block* block = &doc->blocks.data[bi];
		if (block->id == id) {
			if (!block->parsed) {
				// Lazy parsing:
				bon_reader br = { block->payload, block->payload_size, 0, doc };
				if (br.nbytes != 0) {
					br_set_err(&br, BON_ERR_TRAILING_DATA);
				}
				if (br.error) {
					if (!doc->error) {
						doc->error = br.error;
					}
					return NULL;
				}
				
				block->parsed = BON_TRUE;
			}
			
			return &block->value;
		}
	}
	
	return NULL;
}

const bon_value* bon_r_root(bon_r_doc* doc)
{
	return bon_r_get_block(doc, 0);
}

const bon_value* bon_r_get_key(const bon_value* val, const char* key)
{
	if (val->type != BON_VALUE_OBJ) {
		fprintf(stderr, "bon_r_get_key: not an object\n");
		return NULL;
	}
	
	const bon_value_obj*  obj  = &val->u.obj;
	const bon_kvs*        kvs  = &obj->kvs;
	
	for (bon_size i=0; i<kvs->size; ++i) {
		const bon_kv* kv = &kvs->data[i];
		
		// The key should always be a string:
		if (kv->key.type == BON_VALUE_STRING) {
			if (memcmp(key, kv->key.u.str.ptr, kv->key.u.str.size) == 0) {
				return &kv->val;
			}
		}
	}
	
	return NULL;
}
	
const char* bon_r_cstr(const bon_value* obj)
{
	if (!obj) {
		return NULL;
	} else if (obj->type == BON_VALUE_STRING) {
		return (const char*)obj->u.str.ptr;
	} else {
		return NULL;
	}
}
	
uint64_t bon_r_uint(bon_r_doc* doc, const bon_value* obj)
{
	if (!obj) {
		bon_r_set_error(doc, BON_ERR_NULL_OBJ);
	} else if (obj->type == BON_VALUE_UINT64) {
		return obj->u.u64;
	} else if (obj->type == BON_VALUE_SINT64) {
		if (obj->u.s64 < 0) {
			bon_r_set_error(doc, BON_ERR_NARROWING);
		} else {
			return (uint64_t)obj->u.s64;
		}
	} else {
		bon_r_set_error(doc, BON_ERR_BAD_TYPE);
	}
	
	return 0; // On error
}
	
bon_size bon_r_list_size(const bon_value* val)
{
	switch (val->type) {
		case BON_VALUE_LIST:
			return val->u.list.values.size;
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = &val->u.agg;
			if (agg->type.id == BON_TYPE_ARRAY) {
				return agg->type.u.array->size;
			} else {
				return 0;
			}
		}
			
		default:
			return 0;
	}
}
	
const bon_value* bon_r_list_elem(const bon_value* val, bon_size ix)
{
	if (val->type == BON_VALUE_LIST)
	{
		const bon_values* vals = &val->u.list.values;
		if (ix < vals->size) {
			return &vals->data[ ix ];
		}
	}
	
	return NULL;
}

///////////////////////////////////////////////////


/*
 from and to aggregate (i.e. structs and array)
 */
bon_bool br_read_aggregate(bon_r_doc* doc, const bon_type* srcType, bon_reader* br,
									const bon_type* dstType, bon_writer* bw)
{
	if (dstType->id == BON_TYPE_ARRAY &&
		 srcType->id == BON_TYPE_ARRAY)
	{
		// array -> array
		bon_type_array* srcArray = srcType->u.array;
		bon_type_array* dstArray = dstType->u.array;
		
		if (srcArray->size != dstArray->size) {
			return BON_FALSE;
		}
		
		for (bon_size ai=0; ai<srcArray->size; ++ai) {
			bon_bool win = br_read_aggregate(doc, srcArray->type, br,
														dstArray->type, bw);
			
			if (!win || doc->error || br->error || bw->error) {
				return BON_FALSE;
			}
		}
		
		return BON_TRUE;
	}
	
	if (dstType->id == BON_TYPE_STRUCT &&
		 srcType->id == BON_TYPE_STRUCT)
	{
		// struct -> struct
		bon_type_struct* srcStruct = srcType->u.strct;
		bon_type_struct* dstStruct = dstType->u.strct;
		
		for (bon_size dst_ki=0; dst_ki<dstStruct->size; ++dst_ki) {
			const char* key = dstStruct->keys[dst_ki];
			
			bon_size src_byte_offset = 0;
			bon_size src_ki;
			for (src_ki=0; src_ki<srcStruct->size; ++src_ki) {
				const bon_type*  srcValType = srcStruct->types[src_ki];
				const bon_size   srcValSize = bon_aggregate_payload_size(srcValType);
				
				if (strcmp(key, srcStruct->keys[src_ki])==0) {
					bon_reader br_val = {
						br->data + src_byte_offset,
						srcValSize,
						br->error,
						0
					};
					
					bon_bool win = br_read_aggregate(doc, srcValType, &br_val,
																dstStruct->types[dst_ki], bw);
					if (!win || doc->error || br_val.error || bw->error) {
						return BON_FALSE;
					}
					
					break; // key found
				}
				
				src_byte_offset += srcValSize;
				if (src_byte_offset > br->nbytes) {
					fprintf(stderr, "Something is seriously wrong\n");
					return BON_FALSE;
				}
			}
			
			if (src_ki == srcStruct->size) {
				fprintf(stderr, "Source lacked key \"%s\"\n", key);
				return BON_FALSE;
			}
		}
		
		// Skip src struct:
		br_skip(br, bon_aggregate_payload_size(srcType));
		
		return BON_TRUE; // All keys read
	}
	
	
	switch (dstType->id) {
		case BON_TYPE_UINT8: {
			if (bw->nbytes < sizeof(uint8_t)) {
				return BON_FALSE;
			}
			*(uint8_t*)bw->data = br_read_uint64(br, srcType->id);
			bw_skip(bw, sizeof(uint8_t));
			return BON_TRUE;
		}
			
		case BON_TYPE_UINT32: {
			if (bw->nbytes < sizeof(uint32_t)) {
				return BON_FALSE;
			}
			*(uint32_t*)bw->data = br_read_uint64(br, srcType->id);
			bw_skip(bw, sizeof(uint32_t));
			return BON_TRUE;
		}
			
			
			// TODO: all cases
			
		case BON_TYPE_FLOAT32: {
			if (bw->nbytes < sizeof(float)) {
				return BON_FALSE;
			}
			*(float*)bw->data = (float)br_read_double(br, srcType->id);
			bw_skip(bw, sizeof(float));
			return BON_TRUE;
		}
			
		case BON_TYPE_FLOAT64: {
			if (bw->nbytes < sizeof(double)) {
				return BON_FALSE;
			}
			*(double*)bw->data = br_read_double(br, srcType->id);
			bw_skip(bw, sizeof(double));
			return BON_TRUE;
		}

			
		default:
			fprintf(stderr, "br_read_aggregate: unknown type\n");
			return BON_FALSE;
	}
}


bon_bool bw_read_aggregate(bon_r_doc* doc, const bon_value* srcVal, const bon_type* dstType, bon_writer* bw)
{
	switch (srcVal->type)
	{
		case BON_VALUE_NIL:
			fprintf(stderr, "bon_r_read_aggregate: nil\n");
			return BON_FALSE; // There is no null-able type
			
		case BON_VALUE_BOOL:
			if (dstType->id!=BON_TYPE_BOOL) {
				fprintf(stderr, "bon_r_read_aggregate: bool err\n");
				return BON_FALSE;
			}
			return bw_write_uint8( bw, srcVal->u.boolean ? BON_TRUE : BON_FALSE );
			
		case BON_VALUE_UINT64: {
			switch (dstType->id) {
				case BON_TYPE_UINT8: {
					return bw_write_uint_as_uint8( bw, srcVal->u.u64 );
					// TODO: handle all these cases
					
				default:
					fprintf(stderr, "Cannot convert from uint64 to %X\n", (int)dstType->id);
					return BON_FALSE;
				}
			}
			
		case BON_VALUE_SINT64: {
			switch (dstType->id) {
				case BON_TYPE_UINT8:
					return bw_write_uint8( bw, srcVal->u.s64 );
					
					// TODO: handle all these cases
					
				default:
					fprintf(stderr, "Cannot convert from sint64\n");
					return BON_FALSE;
			}
		}
			
			
		case BON_VALUE_DOUBLE: {
			switch (dstType->id) {
					// TODO: handle all these cases
					
				default:
					fprintf(stderr, "Cannot convert from double\n");
					return BON_FALSE;
			}
		}
			
			
		case BON_VALUE_STRING:
			if (dstType->id != BON_TYPE_STRING) {
				return BON_FALSE;
			}
			if (bw->nbytes != sizeof(const char*)) {
				return BON_FALSE;
			}
			*(const char**)bw->data  = (const char*)srcVal->u.str.ptr;
			return bw_skip(bw, sizeof(const char*));
			
			
		case BON_VALUE_LIST: {
			fprintf(stderr, "Cannot auto-convert to list\n");
			return BON_FALSE;
		}
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = &srcVal->u.agg;
			bon_size byteSize = bon_aggregate_payload_size(&agg->type);
			bon_reader br = { agg->data, byteSize, 0, 0 };
			bon_bool win = br_read_aggregate(doc, &agg->type, &br, dstType, bw);
			return win && br.error==0;
		}
			
			
		case BON_VALUE_OBJ: {
			if (dstType->id != BON_TYPE_STRUCT) {
				return BON_FALSE;
			}
			
			// For every dst key, find that in src:
			const bon_type_struct* strct = dstType->u.strct;
			for (bon_size ki=0; ki<strct->size; ++ki) {
				const char*       key = strct->keys[ki];
				const bon_value*  val = bon_r_get_key(srcVal, key);
				
				if (val == NULL) {
					fprintf(stderr, "Failed to find key in src: \"%s\"\n", key);
					return BON_FALSE;
				}
				
				const bon_type* kv_type = strct->types[ki];
				
				bon_size nByteInVal = bon_aggregate_payload_size(kv_type);
				if (bw->nbytes < nByteInVal) {
					return BON_FALSE;
				}
				bw_read_aggregate(doc, val, kv_type, bw);
			}
			
			return bw->nbytes == 0; // There should be none left
		}
			
			
		case BON_VALUE_BLOCK_REF: {
			const bon_value* val = bon_r_get_block(doc, srcVal->u.blockRefId );
			if (val) {
				return bw_read_aggregate(doc, val, dstType, bw);
			} else {
				fprintf(stderr, "Missing block, id %"PRIu64 "\n", srcVal->u.blockRefId );
				return BON_FALSE;
			}
		}
		}
	}
}

bon_bool bon_r_read_aggregate(bon_r_doc* doc, const bon_value* srcVal, const bon_type* dstType, void* dst, bon_size nbytes)
{
	bon_size expected = bon_aggregate_payload_size(dstType);
	if (expected != nbytes) {
		fprintf(stderr, "destination type and buffer size does not match. Expected %d, got %d\n", (int)expected, (int)nbytes);
		return BON_FALSE;
	}
	
	bon_writer bw = {dst, nbytes, 0};
	bon_bool win = bw_read_aggregate(doc, srcVal, dstType, &bw);
	return win && bw.nbytes==0 && bw.error==0;
}
	
// Convenience:
bon_bool bon_r_read_aggregate_fmt(bon_r_doc* doc, const bon_value* srcVal,
											 void* dst, bon_size nbytes, const char* fmt)
{
	// TODO: var args ...
	bon_type* type = bon_new_type_fmt(fmt);
	if (!type) {
		return BON_FALSE;
	}
	bon_bool win = bon_r_read_aggregate(doc, srcVal, type, dst, nbytes);
	bon_free_type(type);
	
	return win;
}

////////////////////////////////////

void bon_print_float(FILE* out, double dbl)
{
	if (isnan(dbl))      fprintf(out, "NaN");
	else if (isinf(dbl)) fprintf(out, dbl < 0 ? "-Inf" : "+Inf");
	else                 fprintf(out, "%f", dbl); // TODO: precision, NaN, Inf
}

void bon_print_aggr(FILE* out, const bon_type* aggType, bon_reader* br)
{
	uint8_t id = aggType->id;
	
	switch (id) {
		case BON_TYPE_ARRAY: {
			fprintf(out, "[");
			bon_type_array* arr = aggType->u.array;
			for (uint64_t ti=0; ti<arr->size; ++ti) {
				bon_print_aggr(out, arr->type, br);
				if (ti != arr->size-1)
					fprintf(out, ", ");
			}
			fprintf(out, "]");
		} break;
			
			
		case BON_TYPE_STRUCT: {
			fprintf(out, "{");
			bon_type_struct* strct = aggType->u.strct;
			for (uint64_t ti=0; ti<strct->size; ++ti) {
				fprintf(out, "\"%s\": ", strct->keys[ti]);
				bon_print_aggr(out, strct->types[ti], br);
				if (ti != strct->size-1)
					fprintf(out, ", ");
			}
			fprintf(out, "}");
		} break;
			
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE: {
			int64_t s64 = br_read_sint64(br, id);
			fprintf(out, "%"PRIi64, s64);
		}
			
			
		case BON_CTRL_UINT8:
		case BON_CTRL_UINT16_LE:
		case BON_CTRL_UINT16_BE:
		case BON_CTRL_UINT32_LE:
		case BON_CTRL_UINT32_BE:
		case BON_CTRL_UINT64_LE:
		case BON_CTRL_UINT64_BE: {
			uint64_t u64 = br_read_uint64(br, id);
			fprintf(out, "%"PRIu64, u64);
		} break;
			
			
		case BON_CTRL_FLOAT32_LE:
		case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:
		case BON_CTRL_FLOAT64_BE: {
			double dbl = br_read_double(br, id);
			bon_print_float(out, dbl);
		} break;
			
			
		default: {
			fprintf(out, "2JSON_ERROR");
		}
	}
}

// Will print in json format.
void bon_print(FILE* out, const bon_value* v, size_t indent)
{
#define INDENT "\t"
#define PRINT_NEWLINE_INDENT fprintf(out, "\n"); for (size_t iix=0; iix<indent; ++iix) fprintf(out, INDENT);
	
	switch (v->type) {
		case BON_VALUE_NIL:
			fprintf(out, "nil");
			break;
			
			
		case BON_VALUE_BOOL:
			fprintf(out, v->u.boolean ? "true" : "false");
			break;
			
			
		case BON_VALUE_UINT64:
			fprintf(out, "%"PRIu64, v->u.u64);
			break;
			
			
		case BON_VALUE_SINT64:
			fprintf(out, "%"PRIi64, v->u.s64);
			break;
			
			
		case BON_VALUE_DOUBLE:
			bon_print_float(out, v->u.dbl);
			break;
			
			
		case BON_VALUE_STRING:
			fprintf(out, "\"");
			fwrite(v->u.str.ptr, v->u.str.size, 1, out);
			fprintf(out, "\"");
			break;
			
			
		case BON_VALUE_LIST: {
			fprintf(out, "[ ");
			const bon_values* vals = &v->u.list.values;
			bon_size size = vals->size;
			for (bon_size i=0; i<size; ++i) {
				bon_print(out, vals->data + i, indent);
				if (i != size-1)
					fprintf(out, ", ");
			}
			fprintf(out, " ]");
		} break;
			
			
		case BON_VALUE_AGGREGATE: {
			bon_size byteSize = bon_aggregate_payload_size(&v->u.agg.type);
			bon_reader br = { v->u.agg.data, byteSize, 0, 0 };
			bon_print_aggr(out, &v->u.agg.type, &br);
			if (br.nbytes!=0) {
				fprintf(stderr, "Bad aggregate\n");
			}
		} break;
			
			
		case BON_VALUE_OBJ: {			
			const bon_kvs* kvs = &v->u.obj.kvs;
			bon_size size = kvs->size;
			
			if (size==0) {
				fprintf(out, "{ }");
			} else {
				bon_bool multiline = (size > 1);
				
				if (multiline) {
					fprintf(out, "{");
					indent++;
					PRINT_NEWLINE_INDENT
				} else {
					fprintf(out, "{ ");
				}
				
				for (bon_size i=0; i<size; ++i) {
					const bon_kv* kv = &kvs->data[i];
					
					bon_print(out, &kv->key, indent);
					fprintf(out, ": ");
					bon_print(out, &kv->val, indent);
					
					if (i != size-1) {
						fprintf(out, ",");
						
						if (multiline) {
							PRINT_NEWLINE_INDENT
						}
					}
				}
				
				if (multiline) {
					indent--;
					PRINT_NEWLINE_INDENT
					fprintf(out, "}");
				} else {
					fprintf(out, " }");
				}
			}
		} break;
			
		case BON_VALUE_BLOCK_REF: {
			fprintf(out, "@%"PRIu64, v->u.blockRefId);
		}
			
		default:
			fprintf(out, "ERROR");
	}
}
