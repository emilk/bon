//
//  read.c
//  BON
//
//  Created by emilk on 2013-04-07.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include "bon.h"
#include "bon_private.h"
#include "crc32.h"
#include <assert.h>
#include <stdarg.h>       // va_list, va_start, va_arg, va_end
#include <stdlib.h>       // malloc, free etc
#include <string.h>       // memcpy


// For fprintf:ing int64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>



//------------------------------------------------------------------------------


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

//------------------------------------------------------------------------------


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

bon_bool bon_is_int(uint8_t ctrl)
{
	switch (ctrl) {
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE:
			
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

//------------------------------------------------------------------------------


// Helper for reading without overflowing:

void br_set_err(bon_reader* br, bon_error err)
{
	bon_onError(bon_err_str(err));
	
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
int br_peek(bon_reader* br) {
	if (br->nbytes > 0) {
		return br->data[0];
	} else {
		return -1;
	}
}

uint8_t br_next(bon_reader* br) {
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
	if (br_next(br) != token) {
		br_set_err(br, BON_ERR_MISSING_TOKEN);
	}
}


//------------------------------------------------------------------------------
// VarInt - standard VLQ
// See http://en.wikipedia.org/wiki/Variable-length_quantity
// See http://rosettacode.org/wiki/Variable-length_quantity

// Maximum number of bytes to encode a very large number
#define BON_VARINT_MAX_LEN 10

uint64_t br_read_vlq(bon_reader* br)
{
	uint64_t r = 0;
	uint32_t size = 0; // Sanity check
	
	for (;;) {
		uint8_t in = br_next(br);
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



//------------------------------------------------------------------------------

void bon_r_list_values(bon_reader* br, bon_value_list* vals);
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
		case BON_TYPE_SINT8:      return (int8_t)br_next(br);
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
		case BON_TYPE_UINT8:      return (uint8_t)br_next(br);
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
	bon_type_array* array = BON_ALLOC_TYPE(1, bon_type_array);
	type->u.array = array;
	array->size = arraySize;
	array->type = (bon_type*)calloc(1, sizeof(bon_type));
	parse_aggr_type(br, array->type);
}

void parse_struct_type(bon_reader* br, bon_type* type, bon_size structSize)
{
	type->id = BON_TYPE_STRUCT;
	bon_type_struct* strct = BON_ALLOC_TYPE(1, bon_type_struct);
	type->u.strct = strct;
	strct->size   = structSize;
	strct->kts    = BON_ALLOC_TYPE(structSize, bon_kt);
	
	for (bon_size ti=0; ti<strct->size; ++ti) {
		bon_value key;
		bon_r_value(br, &key);
		if (key.type != BON_VALUE_STRING) {
			br_set_err(br, BON_ERR_BAD_KEY);
			return;
		}
		
		bon_kt* kt = &strct->kts[ti];
		kt->key = (const char*)key.u.str.ptr;
		parse_aggr_type(br, &kt->type);
	}
}

void parse_aggr_type(bon_reader* br, bon_type* type)
{
	memset(type, 0, sizeof(bon_type));
	
	if (br->error)
		return;
	
	uint8_t ctrl = br_next(br);
	
	if (BON_SHORT_AGGREGATES_START <= ctrl   &&  ctrl < BON_SHORT_NEG_INT_START)
	{
		// Compressed array or struct:
		if (ctrl  >=  BON_SHORT_STRUCT_START)
		{
			bon_size structSize = ctrl - BON_SHORT_STRUCT_START;
			parse_struct_type(br, type, structSize);
		}
		else if (ctrl  >=  BON_SHORT_BYTE_ARRAY_START)
		{
			bon_size arraySize     = ctrl - BON_SHORT_BYTE_ARRAY_START;
			
			bon_type_array* array  = BON_ALLOC_TYPE(1, bon_type_array);
			array->size            = arraySize;
			array->type            = (bon_type*)calloc(1, sizeof(bon_type));
			array->type->id        = BON_TYPE_UINT8;
			
			type->id               = BON_TYPE_ARRAY;
			type->u.array          = array;
		}
		else
		{
			assert(ctrl >= BON_SHORT_ARRAY_START);
			bon_size arraySize = ctrl - BON_SHORT_ARRAY_START;
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

void bon_r_unpack_value(bon_reader* br, bon_value* val)
{
	val->type = BON_VALUE_AGGREGATE;
	bon_type* type = &val->u.agg.type;
	parse_aggr_type(br, type);
	val->u.agg.data = br->data;
	bon_size nBytesPayload = bon_aggregate_payload_size(type);
	br_skip(br, nBytesPayload);
	
	if (br->B) {
		br->B->stats.count_aggr      +=  1;
		br->B->stats.bytes_aggr_dry  +=  nBytesPayload;
		//br->B->stats.bytes_aggr_wet  += (bytes_left_start - br->nbytes);
	}
}

void bon_r_string_sized(bon_reader* br, bon_value* val, size_t strLen)
{
	val->type       = BON_VALUE_STRING;
	val->u.str.size = strLen;
	val->u.str.ptr  = (const char*)br->data;
	br_skip(br, strLen);
	int zero = br_next(br);
	
	if (zero != 0) {
		br_set_err(br, BON_ERR_STRING_NOT_ZERO_ENDED);
	}
	
	if (br->B) {
		br->B->stats.count_string      += 1;
		br->B->stats.bytes_string_dry  += val->u.str.size;
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
			br_assert(br, val->u.blockRefId > br->block_id, BON_ERR_BAD_BLOCK_REF);
			break;
			
			
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
			bon_r_list_values(br, &val->u.list);
			br_swallow(br, BON_CTRL_LIST_END);
			break;
			
			
		case BON_CTRL_OBJ_BEGIN:
			val->type      = BON_VALUE_OBJ;
			bon_r_kvs(br, &val->u.obj.kvs);
			if (br_next(br) != BON_CTRL_OBJ_END) {
				br_set_err(br, BON_ERR_MISSING_OBJ_END);
			}
			break;
			
			
		case BON_CTRL_ARRAY_VLQ:
		case BON_CTRL_TUPLE_VLQ:
		case BON_CTRL_STRUCT_VLQ: {
			br_putback(br);
			bon_r_unpack_value(br, val);
		} break;
			
			
		default: {
			br_set_err(br, BON_ERR_BAD_CTRL);
		}
	}
}


void bon_r_value(bon_reader* br, bon_value* val)
{
	memset(val, 0, sizeof(bon_value));
	
	uint8_t ctrl = br_next(br);
	
	
	if     (ctrl  >=  BON_SHORT_NEG_INT_START)
	{
		val->type   = BON_VALUE_SINT64;
		val->u.s64  = (int8_t)ctrl;
	}
	else if (ctrl >= BON_SHORT_AGGREGATES_START)
	{
		br_putback(br);
		bon_r_unpack_value(br, val);
	}
	else if (ctrl  >=  BON_SHORT_BLOCK_START)
	{
		val->type          = BON_VALUE_BLOCK_REF;
		val->u.blockRefId  = ctrl - BON_SHORT_BLOCK_START;
		br_assert(br, val->u.blockRefId > br->block_id, BON_ERR_BAD_BLOCK_REF);
	}
	else if (ctrl  >=  BON_SHORT_CODES_START)
	{
		bon_r_value_from_ctrl(br, val, ctrl);
	}
	else if (ctrl  >=  BON_SHORT_STRING_START)
	{
		// FixString
		bon_r_string_sized(br, val, ctrl - BON_SHORT_STRING_START);
	}
	else
	{
		// PosFixNum
		assert(ctrl < 32);
		val->type   = BON_VALUE_UINT64;
		val->u.u64  = (uint64_t)ctrl;
	}
}


void bon_r_list_values(bon_reader* br, bon_value_list* vals)
{
	memset(vals, 0, sizeof(bon_value_list));
	
	while (!br->error)
	{
		if (br_peek(br) == BON_CTRL_LIST_END) {
			break;
		}
		
		BON_VECTOR_EXPAND(*vals, bon_value, 1)
		bon_r_value(br, vals->data + vals->size - 1);
	}
}


bon_value* bon_r_load_block(bon_r_doc* B, uint64_t id);


void bon_r_kvs(bon_reader* br, bon_kvs* kvs)
{
	memset(kvs, 0, sizeof(bon_kvs));
	
	while (!br->error)
	{
		int ctrl = br_peek(br);
		if (ctrl == BON_CTRL_OBJ_END) {
			break;
		}
		
		BON_VECTOR_EXPAND(*kvs, bon_kv, 1)
		
		bon_kv* kv = kvs->data + kvs->size - 1;
		
		bon_value keyVal;
		bon_r_value(br, &keyVal);
		
		const bon_value* key = &keyVal;
		
		if (key->type == BON_VALUE_BLOCK_REF) {
			key = bon_r_load_block(br->B, key->u.blockRefId);
			if (!key) {
				br_set_err(br, BON_ERR_BAD_KEY);
				return;
			}
		}
		
		// Check key is string without hidden zeros:
		if (key->type       != BON_VALUE_STRING ||
			 key->u.str.size != strlen(key->u.str.ptr))
		{
			br_set_err(br, BON_ERR_BAD_KEY);
			return;
		}
		
		kv->key = key->u.str.ptr;
		bon_r_value(br, &kv->val);
	}
}

void bon_r_header(bon_reader* br)
{
	if (br_peek(br) == BON_CTRL_HEADER)
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
	else
	{
		br_set_err(br, BON_ERR_BAD_HEADER);
		//fprintf(stderr, "Expected BON0, got first byte 0xX\n", (unsigned)br_peek(br));
	}
}

void bon_r_footer(bon_reader* br)
{
	if (br_peek(br) == BON_CTRL_FOOTER)
	{
		br_skip(br, 1);
	}
	else if (br_peek(br) == BON_CTRL_FOOTER_CRC)
	{
		br_skip(br, 1);
		br_skip(br, 4); // ignore crc
		br_swallow(br, BON_CTRL_FOOTER_CRC);
	}
	
	br_assert(br, br->nbytes==0, BON_ERR_TRAILING_DATA);
}

void bon_r_read_content(bon_reader* br)
{
	bon_r_blocks* blocks = &br->B->blocks;
	
	if (br_peek(br) == BON_CTRL_BLOCK_BEGIN)
	{
		// Blocked document
		
		while (!br->error && br_peek(br)==BON_CTRL_BLOCK_BEGIN) {
			BON_VECTOR_EXPAND(*blocks, bon_r_block, 1);
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
				
				bon_reader block_br = {
					br->data,
					br->nbytes,
					br->B,
					block->id,
					br->error
				};
				bon_r_value(&block_br, &block->value);
				bon_size nRead = br->nbytes - block_br.nbytes;
				block->payload_size = nRead;
				br_skip(br, nRead);
				
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
		blocks->data  = BON_ALLOC_TYPE(1, bon_r_block);
		bon_r_block* root = blocks->data;
		root->id              = 0;
		root->payload         = br->data;
		root->payload_size    = 0;
		root->parsed          = BON_TRUE;
		bon_r_value(br, &root->value);
	}
}

void bon_r_read_doc(bon_reader* br)
{
	bon_r_doc* B = br->B;
	B->stats.bytes_file = br->nbytes;
	bon_r_header(br);
	if (br->error) return;
	bon_r_read_content(br);
	if (br->error) return;
	bon_r_footer(br);
}

void bon_r_set_error(bon_r_doc* B, bon_error err)
{
	bon_onError(bon_err_str(err));
	
	if (B->error == BON_SUCCESS) {
		B->error = err;
	}
}

bon_r_doc* bon_r_open(const uint8_t* data, bon_size nbytes, bon_r_flags flags)
{
	assert(data);
	
	bon_r_doc* B = BON_ALLOC_TYPE(1, bon_r_doc);
	B->flags = flags;
	
	if (B->flags & BON_R_FLAG_REQUIRE_CRC)
	{
		/*
		 The last six bytes of the file should be:
		 
		 BON_CTRL_FOOTER_CRC
		 crc32_le
		 BON_CTRL_FOOTER_CRC
		 
		 and the crc covers everything except these six bytes.
		 */
		if (nbytes < 6 ||
			 data[nbytes-1] != BON_CTRL_FOOTER_CRC ||
			 data[nbytes-6] != BON_CTRL_FOOTER_CRC)
		{
			bon_r_set_error(B, BON_ERR_BAD_CRC);
			return B;
		}
		else
		{
			uint32_t crc_calced  =  crc_calc(data, nbytes-6);
			uint32_t crc_read_le;
			memcpy(&crc_read_le, data + nbytes - 5, 4);
			uint32_t crc_read = le_to_uint32(crc_read_le);
			
			if (crc_calced != crc_read) {
				bon_r_set_error(B, BON_ERR_BAD_CRC);
				return B;
			}
		}
	}
	
	bon_reader br_v = { data, nbytes, B, BON_BAD_BLOCK_ID, 0 };
	bon_reader* br = &br_v;
	
	bon_r_read_doc(br);
	
	B->error = br->error;
	return B;
}

void bon_free_value_insides(bon_value* val)
{
	if (!val) { return; }
	
	switch (val->type)
	{
		case BON_VALUE_LIST: {
			for (bon_size ix=0; ix < val->u.list.size; ++ix) {
				bon_free_value_insides( val->u.list.data + ix );
			}
		} break;
			
		case BON_VALUE_OBJ: {
			bon_kvs* kvs = &val->u.obj.kvs;
			for (bon_size ix=0; ix<kvs->size; ++ix) {
				bon_free_value_insides( &kvs->data[ix].val );
			}
		} break;
			
		case BON_VALUE_AGGREGATE: {
			bon_free_type_insides( &val->u.agg.type );
			if (val->u.agg.exploded) {
				bon_free_value_insides( val->u.agg.exploded );
				free( val->u.agg.exploded );
			}
		} break;
			
		default:
			break;
	}
}

void bon_r_close(bon_r_doc* B)
{
	for (bon_size bi=0; bi<B->blocks.size; ++bi) {
		bon_r_block* block = B->blocks.data + bi;
		if (block->parsed) {
			bon_free_value_insides( &block->value );
		}
	}
	
	
	free(B);
}

// Returns NULL on fail
bon_r_block* bon_r_find_block(bon_r_doc* B, uint64_t id)
{
	for (bon_size bi=0; bi<B->blocks.size; ++bi) {
		bon_r_block* block = &B->blocks.data[bi];
		if (block->id == id) {
			return block;
		}
	}
	return NULL;
}

// Returns NULL on fail
bon_value* bon_r_load_block(bon_r_doc* B, uint64_t id)
{
	bon_r_block* block = bon_r_find_block(B, id);
	
	if (!block->parsed) {
		// Lazy parsing:
		bon_reader br = { block->payload, block->payload_size, B, id, 0 };
		
		bon_r_value(&br, &block->value);
		
		if (br.nbytes != 0) {
			br_set_err(&br, BON_ERR_TRAILING_DATA);
		}
		if (br.error) {
			if (!B->error) {
				B->error = br.error;
			}
			return NULL;
		}
		
		block->parsed = BON_TRUE;
	}
	
	return &block->value;
}

// Returns NULL on fail
bon_value* bon_r_get_block(bon_r_doc* B, bon_block_id id)
{
	return bon_r_load_block(B, id);
}

bon_value* bon_r_root(bon_r_doc* B)
{
	return bon_r_get_block(B, 0);
}

bon_error bon_r_error(bon_r_doc* B)
{
	return B->error;
}

bon_value* bon_r_follow_refs(bon_r_doc* B, bon_value* val)
{
	/* We should be protected from infinite recursion here,
	 since we check all blockRefId on reading,
	 ensuring they only point to blocks with higher ID:s. */
	while (B->error==0 &&
			 val && val->type == BON_VALUE_BLOCK_REF)
	{
		val = bon_r_get_block(B, val->u.blockRefId);
	}
	return val;
}

//------------------------------------------------------------------------------

bon_bool bon_r_is_nil(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_NIL;
}

bon_bool bon_r_is_bool(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_BOOL;
}

bon_bool bon_r_is_int(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	return val->type == BON_VALUE_SINT64
	||     val->type == BON_VALUE_UINT64;
}

bon_bool bon_r_is_number(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	return val->type == BON_VALUE_SINT64
	||     val->type == BON_VALUE_UINT64
	||     val->type == BON_VALUE_DOUBLE;
}

bon_bool bon_r_is_double(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_DOUBLE;
}

bon_bool bon_r_is_string(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_STRING;
}

bon_bool bon_r_is_list(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	if (val->type == BON_VALUE_LIST) { return BON_TRUE; }
	if (val->type == BON_VALUE_AGGREGATE) {
		const bon_value_agg* agg = &val->u.agg;
		return agg->type.id == BON_TYPE_ARRAY;
	}
	return BON_FALSE;
}

bon_bool bon_r_is_object(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	if (val->type == BON_VALUE_OBJ) { return BON_TRUE; }
	if (val->type == BON_VALUE_AGGREGATE) {
		const bon_value_agg* agg = &val->u.agg;
		return agg->type.id == BON_TYPE_STRUCT;
	}
	return BON_FALSE;
}


bon_logical_type  bon_r_value_type(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_LOGICAL_ERROR; }
	
	switch (val->type)
	{
		case BON_VALUE_NIL:     { return BON_LOGICAL_NIL; }
		case BON_VALUE_BOOL:    { return BON_LOGICAL_BOOL; }
		case BON_VALUE_UINT64:  { return BON_LOGICAL_UINT; }
		case BON_VALUE_SINT64:  { return BON_LOGICAL_SINT; }         
		case BON_VALUE_DOUBLE:  { return BON_LOGICAL_DOUBLE; }
		case BON_VALUE_STRING:  { return BON_LOGICAL_STRING; }
		case BON_VALUE_LIST:    { return BON_LOGICAL_LIST; }
		case BON_VALUE_OBJ:     { return BON_LOGICAL_OBJECT; }
			
		case BON_VALUE_AGGREGATE: {
			if (bon_r_is_list(B, val)) {
				return BON_LOGICAL_LIST;
			} else if (bon_r_is_object(B, val)) {
				return BON_LOGICAL_OBJECT;
			} else {
				return BON_LOGICAL_ERROR;
			}
		}
			
		default: {
			return BON_LOGICAL_ERROR;
		}
	}
}

//------------------------------------------------------------------------------

bon_bool bon_r_bool(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_BOOL) {
		return val->u.boolean;
	} else {
		return BON_FALSE;
	}
}

int64_t bon_r_int(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_UINT64) {
		return (int64_t)val->u.u64;
	} else if (val->type == BON_VALUE_SINT64) {
		return val->u.s64;
	} else if (val->type == BON_VALUE_DOUBLE) {
		return (int64_t)val->u.dbl;
	} else {
		return 0;
	}
}

uint64_t bon_r_uint(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_UINT64) {
		return val->u.u64;
	} else {
		return (uint64_t)bon_r_int(B, val);
	}
}

double bon_r_double(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_DOUBLE) {
		return val->u.dbl;
	} else if (val->type == BON_VALUE_UINT64) {
		return val->u.u64;
	} else if (val->type == BON_VALUE_SINT64) {
		return val->u.s64;
	} else {
		return 0;
	}
}

float bon_r_float(bon_r_doc* B, bon_value* obj)
{
	return (float)bon_r_double(B, obj);
}

bon_size bon_r_strlen(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_STRING) {
		return val->u.str.size;
	} else {
		return 0;
	}
}

const char* bon_r_cstr(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return NULL;
	} else if (val->type == BON_VALUE_STRING) {
		return (const char*)val->u.str.ptr;
	} else {
		return NULL;
	}
}

bon_size bon_r_list_size(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return 0; }
	
	switch (val->type) {
		case BON_VALUE_LIST:
			return val->u.list.size;
			
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


bon_bool bon_explode_aggr(bon_r_doc* B, bon_value* dst,
								  bon_type* type, bon_reader* br)
{
	bon_type_id id = type->id;
	
	switch (id)
	{
		case BON_TYPE_ARRAY: {
			bon_type_array* array  =  type->u.array;
			bon_size n             =  array->size;
			
			dst->type              =  BON_VALUE_LIST;
			
			bon_value_list* list   =  &dst->u.list;
			list->size             =  n;
			list->cap              =  n;
			list->data             =  BON_ALLOC_TYPE(n, bon_value);
			
			for (bon_size ix=0; ix<n; ++ix) {
				bon_explode_aggr(B, list->data +ix, array->type, br);
			}
			return BON_TRUE;
		}
			
			
		case BON_TYPE_STRUCT: {
			bon_type_struct* strct  =  type->u.strct;
			bon_size n              =  strct->size;
			
			dst->type               =  BON_VALUE_OBJ;
			
			bon_value_obj*  obj     =  &dst->u.obj;
			bon_kvs*        kvs     =  &obj->kvs;
			kvs->size               =  n;
			kvs->cap                =  n;
			kvs->data               =  BON_ALLOC_TYPE(n, bon_kv);
			
			for (bon_size ix=0; ix<n; ++ix) {
				bon_kv* kv = kvs->data  + ix;
				bon_kt* kt = strct->kts + ix;
				kv->key = kt->key;
				bon_explode_aggr(B, &kv->val, &kt->type, br);
			}
			return BON_TRUE;
		}
			
			
		case BON_TYPE_SINT8:
		case BON_TYPE_SINT16_LE:  case BON_TYPE_SINT16_BE:
		case BON_TYPE_SINT32_LE:  case BON_TYPE_SINT32_BE:
		case BON_TYPE_SINT64_LE:  case BON_TYPE_SINT64_BE: {
			dst->type   =  BON_VALUE_SINT64;
			dst->u.s64  =  br_read_sint64(br, id);
			return BON_TRUE;
		}
			
			
		case BON_TYPE_UINT8:
		case BON_TYPE_UINT16_LE:  case BON_TYPE_UINT16_BE:
		case BON_TYPE_UINT32_LE:  case BON_TYPE_UINT32_BE:
		case BON_TYPE_UINT64_LE:  case BON_TYPE_UINT64_BE: {
			dst->type   =  BON_VALUE_UINT64;
			dst->u.u64  =  br_read_uint64(br, id);
			return BON_TRUE;
		}
			
			
		case BON_TYPE_FLOAT32_LE:  case BON_TYPE_FLOAT32_BE:
		case BON_TYPE_FLOAT64_LE:  case BON_TYPE_FLOAT64_BE: {
			dst->type   =  BON_VALUE_DOUBLE;
			dst->u.dbl  =  br_read_double(br, id);
			return BON_TRUE;
		}
			
			
		default: {
			br_set_err(br, BON_ERR_BAD_AGGREGATE_TYPE);
			return BON_FALSE;
		}
	}
}


bon_value* bon_exploded_aggr(bon_r_doc* B, bon_value* val)
{
	if (!val || val->type != BON_VALUE_AGGREGATE) {
		return val; // Original is good
	}
	
	bon_value_agg* agg = &val->u.agg;
	
	if (!agg->exploded) {
		bon_reader     br  = {
			agg->data,
			bon_aggregate_payload_size(&agg->type),
			B,
			BON_BAD_BLOCK_ID,
			0
		};
		
		agg->exploded = BON_ALLOC_TYPE(1, bon_value);
		
		if (!bon_explode_aggr( B, agg->exploded, &agg->type, &br )) {
			free( agg->exploded );
			agg->exploded = NULL;
			return BON_FALSE;
		}
		
		assert(br.error == 0);
		assert(br.nbytes == 0);
	}
	
	return agg->exploded;
}


bon_value* follow_and_explode(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_AGGREGATE) {
		// If the user treats it like an object, convert it into one.
		val = bon_exploded_aggr(B, val);
	}
	
	return val;
}

bon_value* bon_r_list_elem(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	if (!val) { return NULL; }
	
	if (val->type == BON_VALUE_LIST)
	{
		const bon_value_list* vals = &val->u.list;
		if (ix < vals->size) {
			return &vals->data[ ix ];
		}
	}
	
	return NULL;
}


// Number of keys-value pairs in an object
bon_size bon_r_obj_size(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return 0; }
	
	switch (val->type) {
		case BON_VALUE_OBJ:
			return val->u.obj.kvs.size;
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = &val->u.agg;
			if (agg->type.id == BON_TYPE_STRUCT) {
				return agg->type.u.strct->size;
			} else {
				return 0;
			}
		}
			
		default:
			return 0;
	}
}

// Return NULL if 'val' is not an object, or ix is out of range.
const char* bon_r_obj_key(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	bon_kvs* kvs = &val->u.obj.kvs;
	if (ix < kvs->size) {
		return kvs->data[ix].key;
	} else {
		return NULL;
	}
}

bon_value* bon_r_obj_value(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	bon_kvs* kvs = &val->u.obj.kvs;
	if (ix < kvs->size) {
		return &kvs->data[ix].val;
	} else {
		return NULL;
	}
}


bon_value* bon_r_get_key(bon_r_doc* B, bon_value* val, const char* key)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	const bon_value_obj*  obj  = &val->u.obj;
	const bon_kvs*        kvs  = &obj->kvs;
	
	for (bon_size i=0; i<kvs->size; ++i) {
		bon_kv* kv = &kvs->data[i];
		
		// The key should always be a string:
		if (strcmp(key, kv->key) == 0) {
			return &kv->val;
		}
	}
	
	return NULL;
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------


// Helper for writing to a binary stream without overflowing:

typedef struct {
	uint8_t*    data;
	bon_size    nbytes;
	bon_error   error;
} bon_writer;

void bw_set_err(bon_writer* bw, bon_error err)
{
	bon_onError(bon_err_str(err));
	
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

// Write bytes in reversed order (i.e. reverse endian)
bon_bool bw_write_raw_reversed(bon_writer* bw, const void* in, size_t n) {
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

typedef enum {
	LE,
	BE
} Endian;

typedef enum {
	SIGNED,
	UNSIGNED
} Signness;

bon_bool bw_write_uint_bytes(bon_writer* bw, uint64_t val, bon_size nBytes,
									  Signness signness, Endian endian)
{
	// TODO: check if 'val' fits into 'nBytes' given 'signness'
	
	if (endian == LE) {
		for (bon_size i=0; i<nBytes; ++i) {
			bw_write_uint8(bw, val & 0xff);
			val >>= 8;
		}
		
		if (val != 0) {
			if (signness == UNSIGNED) {
				fprintf(stderr, "WARNING: Value did not fit into destination");
				return BON_FALSE;
			} else {
				// These should be a lot of 0xff left
			}
		}
		
		return BON_TRUE;
	} else {
		for (bon_size i=0; i<nBytes; ++i) {
			bon_size shift = 8 * (nBytes - i - 1);
			bw_write_uint8(bw, (val >> shift) & 0xff);
		}
		
		return BON_TRUE;
	}
}


bon_bool bw_write_double_as(bon_writer* bw, double val, bon_type_id type);

bon_bool bw_write_uint_as(bon_writer* bw, uint64_t val, bon_type_id type)
{
	switch (type) {
		case BON_TYPE_SINT8:      return bw_write_uint_bytes(bw, val, 1, SIGNED,    LE);
		case BON_TYPE_UINT8:      return bw_write_uint_bytes(bw, val, 1, UNSIGNED,  LE);
			
		case BON_TYPE_SINT16_LE:  return bw_write_uint_bytes(bw, val, 2, SIGNED,    LE);
		case BON_TYPE_SINT16_BE:  return bw_write_uint_bytes(bw, val, 2, SIGNED,    BE);
		case BON_TYPE_UINT16_LE:  return bw_write_uint_bytes(bw, val, 2, UNSIGNED,  LE);
		case BON_TYPE_UINT16_BE:  return bw_write_uint_bytes(bw, val, 2, UNSIGNED,  BE);
			
		case BON_TYPE_SINT32_LE:  return bw_write_uint_bytes(bw, val, 4, SIGNED,    LE);
		case BON_TYPE_SINT32_BE:  return bw_write_uint_bytes(bw, val, 4, SIGNED,    BE);
		case BON_TYPE_UINT32_LE:  return bw_write_uint_bytes(bw, val, 4, UNSIGNED,  LE);
		case BON_TYPE_UINT32_BE:  return bw_write_uint_bytes(bw, val, 4, UNSIGNED,  BE);
			
		case BON_TYPE_SINT64_LE:  return bw_write_uint_bytes(bw, val, 8, SIGNED,    LE);
		case BON_TYPE_SINT64_BE:  return bw_write_uint_bytes(bw, val, 8, SIGNED,    BE);
		case BON_TYPE_UINT64_LE:  return bw_write_uint_bytes(bw, val, 8, UNSIGNED,  LE);
		case BON_TYPE_UINT64_BE:  return bw_write_uint_bytes(bw, val, 8, UNSIGNED,  BE);
			
		case BON_TYPE_FLOAT32_LE: case BON_TYPE_FLOAT32_BE:
		case BON_TYPE_FLOAT64_LE: case BON_TYPE_FLOAT64_BE:
			return bw_write_double_as(bw, (double)val, type);
			
		default:
			return BON_FALSE;
	}
}

bon_bool bw_write_sint_as(bon_writer* bw, int64_t val, bon_type_id type)
{
	if (val >= 0) {
		return bw_write_uint_as(bw, (uint64_t)val, type);
	}
	
	switch (type) {
		case BON_TYPE_SINT8:      return bw_write_uint_bytes(bw, (uint64_t)val, 1, SIGNED,    LE);
			
		case BON_TYPE_SINT16_LE:  return bw_write_uint_bytes(bw, (uint64_t)val, 2, SIGNED,    LE);
		case BON_TYPE_SINT16_BE:  return bw_write_uint_bytes(bw, (uint64_t)val, 2, SIGNED,    BE);
			
		case BON_TYPE_SINT32_LE:  return bw_write_uint_bytes(bw, (uint64_t)val, 4, SIGNED,    LE);
		case BON_TYPE_SINT32_BE:  return bw_write_uint_bytes(bw, (uint64_t)val, 4, SIGNED,    BE);
			
		case BON_TYPE_SINT64_LE:  return bw_write_uint_bytes(bw, (uint64_t)val, 8, SIGNED,    LE);
		case BON_TYPE_SINT64_BE:  return bw_write_uint_bytes(bw, (uint64_t)val, 8, SIGNED,    BE);
			
		case BON_TYPE_FLOAT32_LE: case BON_TYPE_FLOAT32_BE:
		case BON_TYPE_FLOAT64_LE: case BON_TYPE_FLOAT64_BE:
			return bw_write_double_as(bw, (double)val, type);
			
		default:
			// Probably an unsigned destination (or a bool, string, ...)
			return BON_FALSE;
	}
}

bon_bool bw_write_double_as(bon_writer* bw, double val, bon_type_id type)
{	
	if (type == BON_TYPE_FLOAT32) {
		float f = (float)val;
		return bw_write_raw(bw, &f, sizeof(f));
	}
	
	if (type == BON_TYPE_FLOAT64){
		return bw_write_raw(bw, &val, sizeof(val));
	}
	
	if (type == BON_TYPE_FLOAT32_LE ||
		 type == BON_TYPE_FLOAT32_BE)
	{
		// non-native endian:
		float f = (float)val;
		return bw_write_raw_reversed(bw, &f, sizeof(f));
	}
	
	if (type == BON_TYPE_FLOAT64_LE ||
		 type == BON_TYPE_FLOAT64_BE)
	{
		// non-native endian:
		return bw_write_raw_reversed(bw, &val, sizeof(val));
	}
	
	if (bon_is_int(type)) {
		return bw_write_sint_as(bw, (int64_t)val, type);
	}
	
	return BON_FALSE;
}


//------------------------------------------------------------------------------

bon_bool translate_aggregate(bon_r_doc* B,
									  const bon_type* srcType, bon_reader* br,
									  const bon_type* dstType, bon_writer* bw);


bon_bool translate_array(bon_r_doc* B,
								 bon_type_array* srcArray, bon_reader* br,
								 bon_type_array* dstArray, bon_writer* bw)
{
	if (srcArray->size != dstArray->size) {
		return BON_FALSE;
	}
	
	for (bon_size ai=0; ai<srcArray->size; ++ai) {
		bon_bool win = translate_aggregate(B, srcArray->type, br,
													  dstArray->type, bw);
		
		if (!win || B->error || br->error || bw->error) {
			return BON_FALSE;
		}
	}
	
	return BON_TRUE;
}


bon_bool translate_struct(bon_r_doc* B,
								  bon_type_struct* srcStruct, bon_reader* br,
								  bon_type_struct* dstStruct, bon_writer* bw)
{
	for (bon_size dst_ki=0; dst_ki<dstStruct->size; ++dst_ki) {
		bon_kt* dst_kt = dstStruct->kts + dst_ki;
		
		bon_size src_byte_offset = 0;
		bon_size src_ki;
		for (src_ki=0; src_ki<srcStruct->size; ++src_ki) {
			bon_kt* src_kt = srcStruct->kts + src_ki;
			const bon_type*  srcValType = &src_kt->type;
			const bon_size   srcValSize = bon_aggregate_payload_size(srcValType);
			
			if (strcmp(dst_kt->key, src_kt->key)==0) {
				bon_reader br_val = {
					br->data + src_byte_offset,
					srcValSize,
					B,
					BON_BAD_BLOCK_ID,
					br->error
				};
				
				bon_bool win = translate_aggregate(B, srcValType, &br_val,
															  &dst_kt->type, bw);
				
				if (!win || B->error || br_val.error || bw->error) {
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
			fprintf(stderr, "Source lacked key \"%s\"\n", dst_kt->key);
			return BON_FALSE;
		}
	}
	
	// Skip src struct:
	br_skip(br, bon_struct_payload_size(srcStruct));
	
	return BON_TRUE; // All keys read
}


/*
 from and to aggregate (i.e. structs and array)
 */
bon_bool translate_aggregate(bon_r_doc* B,
									  const bon_type* srcType, bon_reader* br,
									  const bon_type* dstType, bon_writer* bw)
{
	if (bon_type_eq(srcType, dstType))
	{
		// Optimization
		
		bon_size size = bon_aggregate_payload_size(dstType);
		if (br->nbytes != size || bw->nbytes < size) {
			return BON_FALSE;
		}
		
		memcpy(bw->data, br->data, size);
		bw_skip(bw, size);
		br_skip(br, size);
		return BON_TRUE;
	}
	
	if (dstType->id == BON_TYPE_ARRAY &&
		 srcType->id == BON_TYPE_ARRAY)
	{
		return translate_array(B,
									  srcType->u.array, br,
									  dstType->u.array, bw);
	}
	
	if (dstType->id == BON_TYPE_STRUCT &&
		 srcType->id == BON_TYPE_STRUCT)
	{
		return translate_struct(B,
										srcType->u.strct, br,
										dstType->u.strct, bw);
		
	}
	
	switch (srcType->id) {
		case BON_TYPE_SINT8:
		case BON_TYPE_SINT16_LE:  case BON_TYPE_SINT16_BE:
		case BON_TYPE_SINT32_LE:  case BON_TYPE_SINT32_BE:
		case BON_TYPE_SINT64_LE:  case BON_TYPE_SINT64_BE: {
			return bw_write_sint_as(bw, br_read_sint64(br, srcType->id), dstType->id);
		}
			
		case BON_TYPE_UINT8:
		case BON_TYPE_UINT16_LE:  case BON_TYPE_UINT16_BE:
		case BON_TYPE_UINT32_LE:  case BON_TYPE_UINT32_BE:
		case BON_TYPE_UINT64_LE:  case BON_TYPE_UINT64_BE: {
			return bw_write_uint_as(bw, br_read_uint64(br, srcType->id), dstType->id);
		}
			
		case BON_TYPE_FLOAT32_LE:  case BON_TYPE_FLOAT32_BE:
		case BON_TYPE_FLOAT64_LE:  case BON_TYPE_FLOAT64_BE: {
			return bw_write_double_as(bw, br_read_double(br, srcType->id), dstType->id);
		}
			
			
		default:
			fprintf(stderr, "translate_aggregate: unknown type\n");
			return BON_FALSE;
	}
}

bon_bool bw_read_aggregate(bon_r_doc* B, bon_value* srcVal,
									const bon_type* dstType, bon_writer* bw);

bon_bool bw_list_2_array(bon_r_doc* B, const bon_value_list* src_list,
								 const bon_type_array* dst_array, bon_writer* bw)
{
	if (src_list->size != dst_array->size) {
		return BON_FALSE;
	}
	
	const bon_size n = dst_array->size;
	
	/* Copies to a uniform numeric array quickly. */
#define COPY_NUMERIC_ARRAY(Type)                            \
/**/     Type* dst      = (Type*)bw->data;                  \
/**/  	bon_value* src  = src_list->data;                  \
/**/                                                        \
/**/    	for (bon_size ix=0; ix<n; ++ix) {                  \
/**/    		if (src->type == BON_VALUE_DOUBLE) {            \
/**/    			*dst = (Type)src->u.dbl;                     \
/**/    		} else if (src->type == BON_VALUE_SINT64) {     \
/**/    			*dst = (Type)src->u.s64;                     \
/**/    		} else if (src->type == BON_VALUE_UINT64) {     \
/**/    			*dst = (Type)src->u.u64;                     \
/**/   		} else {                                        \
/**/    			return BON_FALSE;                            \
/**/    		}                                               \
/**/                                                        \
/**/    		++dst;                                          \
/**/    		++src;                                          \
/**/    	}                                                  \
/**/                                                        \
/**/    	bw_skip(bw, n * sizeof(Type));
	
	// Optimizations for copying to native numeric types:
	
	switch (dst_array->type->id) {
		case BON_TYPE_FLOAT32: {
			COPY_NUMERIC_ARRAY(float)
		} break;
			
		case BON_TYPE_FLOAT64: {
			COPY_NUMERIC_ARRAY(double)
		} break;
			
		case BON_TYPE_SINT8: {
			COPY_NUMERIC_ARRAY(int8_t)
		} break;
			
		case BON_TYPE_UINT8: {
			COPY_NUMERIC_ARRAY(uint8_t)
		} break;
			
		case BON_TYPE_SINT16: {
			COPY_NUMERIC_ARRAY(int16_t)
		} break;
			
		case BON_TYPE_UINT16: {
			COPY_NUMERIC_ARRAY(uint16_t)
		} break;
			
		case BON_TYPE_SINT32: {
			COPY_NUMERIC_ARRAY(int32_t)
		} break;
			
		case BON_TYPE_UINT32: {
			COPY_NUMERIC_ARRAY(uint32_t)
		} break;
			
		case BON_TYPE_SINT64: {
			COPY_NUMERIC_ARRAY(int64_t)
		} break;
			
		case BON_TYPE_UINT64: {
			COPY_NUMERIC_ARRAY(uint64_t)
		} break;
	
			
		default: {
			// Maybe a nested type, maybe wrong endian. Recurse.
			
			for (bon_size ix=0; ix<n; ++ix) {
				bw_read_aggregate(B, src_list->data + ix,
										dst_array->type, bw);
			}
		}
	}
	
	return bw->nbytes == 0; // There should be none left
}


bon_bool bw_read_aggregate(bon_r_doc* B, bon_value* srcVal,
									const bon_type* dstType, bon_writer* bw)
{
	switch (srcVal->type)
	{
		case BON_VALUE_DOUBLE:
			return bw_write_double_as( bw, srcVal->u.dbl, dstType->id );
			
			
		case BON_VALUE_SINT64: {
			return bw_write_sint_as( bw, srcVal->u.s64, dstType->id );
			
			
		case BON_VALUE_UINT64:
			return bw_write_uint_as( bw, srcVal->u.u64, dstType->id );
			
			
		case BON_VALUE_LIST: {
			if (dstType->id != BON_TYPE_ARRAY) {
				return BON_FALSE;
			}
			
			const bon_value_list* src_list  = &srcVal->u.list;
			const bon_type_array* dst_array = dstType->u.array;
			
			return bw_list_2_array(B, src_list, dst_array, bw);
		}
			
			
		case BON_VALUE_OBJ: {
			if (dstType->id != BON_TYPE_STRUCT) {
				return BON_FALSE;
			}
			
			// For every dst key, find that in src:
			const bon_type_struct* strct = dstType->u.strct;
			for (bon_size ki=0; ki<strct->size; ++ki) {
				bon_kt* kt = strct->kts + ki;
				const char* key  =  kt->key;
				bon_value*  val  =  bon_r_get_key(B, srcVal, key);
				
				if (val == NULL) {
					fprintf(stderr, "Failed to find key in src: \"%s\"\n", key);
					return BON_FALSE;
				}
				
				const bon_type* kv_type = &kt->type;
				
				bon_size nByteInVal = bon_aggregate_payload_size(kv_type);
				if (bw->nbytes < nByteInVal) {
					return BON_FALSE;
				}
				bw_read_aggregate(B, val, kv_type, bw);
			}
			
			return bw->nbytes == 0; // There should be none left
		}
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = &srcVal->u.agg;
			bon_size byteSize = bon_aggregate_payload_size(&agg->type);
			bon_reader br = { agg->data, byteSize, B, BON_BAD_BLOCK_ID, 0 };
			bon_bool win = translate_aggregate(B, &agg->type, &br, dstType, bw);
			return win && br.error==0;
		}
			
			
		case BON_VALUE_BLOCK_REF: {
			bon_value* val = bon_r_get_block(B, srcVal->u.blockRefId );
			if (val) {
				return bw_read_aggregate(B, val, dstType, bw);
			} else {
				fprintf(stderr, "Missing block, id %"PRIu64 "\n", srcVal->u.blockRefId );
				return BON_FALSE;
			}
		}
						
			
		case BON_VALUE_NIL:
			fprintf(stderr, "bon_r_unpack: nil\n");
			return BON_FALSE; // There is no null-able type
			
			
		case BON_VALUE_BOOL:
			if (dstType->id!=BON_TYPE_BOOL) {
				fprintf(stderr, "bon_r_unpack: bool err\n");
				return BON_FALSE;
			}
			return bw_write_uint8( bw, srcVal->u.boolean ? BON_TRUE : BON_FALSE );
			
			
		case BON_VALUE_STRING: {
			if (dstType->id != BON_TYPE_STRING) {
				return BON_FALSE;
			}
			if (bw->nbytes != sizeof(const char*)) {
				return BON_FALSE;
			}
			*(const char**)bw->data  = (const char*)srcVal->u.str.ptr;
			return bw_skip(bw, sizeof(const char*));
		}
			
			
		default:
			fprintf(stderr, "bon_r_unpack: unknown type: 0x%0X\n", srcVal->type);
			return BON_FALSE;
		}
	}
}

bon_bool bon_r_unpack(bon_r_doc* B, bon_value* srcVal,
								 void* dst, bon_size nbytes,
								 const bon_type* dstType)
{
	const void* src = bon_r_unpack_ptr(B, srcVal, nbytes, dstType);
	
	if (src) {
		// Perfectly matching types
		memcpy(dst, src, nbytes);
		return BON_TRUE;
	} else {
		bon_size expected = bon_aggregate_payload_size(dstType);
		if (expected != nbytes) {
			fprintf(stderr, "destination type and buffer size does not match. Expected %d, got %d\n", (int)expected, (int)nbytes);
			return BON_FALSE;
		}
		
		bon_writer bw = {dst, nbytes, 0};
		bon_bool win = bw_read_aggregate(B, srcVal, dstType, &bw);
		return win && bw.nbytes==0 && bw.error==0;
	}
}

// Convenience:
bon_bool bon_r_unpack_fmt(bon_r_doc* B, bon_value* srcVal,
									  void* dst, bon_size nbytes,
									  const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bon_type* type = bon_new_type_fmt_ap(&fmt, &ap);
	va_end(ap);
	
	if (!type) {
		return BON_FALSE;
	}
	bon_bool win = bon_r_unpack(B, srcVal, dst, nbytes, type);
	bon_free_type(type);
	
	return win;
}

const void* bon_r_unpack_ptr(bon_r_doc* B, bon_value* val,
									bon_size nbytes, const bon_type* dstType)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return NULL; }
	
	if (val->type != BON_VALUE_AGGREGATE) { return NULL; }
	
	if (bon_type_eq(&val->u.agg.type, dstType) == BON_FALSE) { return NULL; }
	
	if (nbytes != bon_aggregate_payload_size(&val->u.agg.type)) {
		fprintf(stderr, "Aggregate size mismatch\n");
		return NULL;
	}
	
	// All win
	return val->u.agg.data;
}

const void* bon_r_unpack_ptr_fmt(bon_r_doc* B, bon_value* val,
										 bon_size nbytes, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bon_type* type = bon_new_type_fmt_ap(&fmt, &ap);
	va_end(ap);
	
	if (!type) {
		return BON_FALSE;
	}
	
	const void* ptr = bon_r_unpack_ptr(B, val, nbytes, type);
	bon_free_type(type);
	
	return ptr;
}


// Quick access to a pointer e.g. pointer of bytes or floats
const void* bon_r_unpack_array(bon_r_doc* B, bon_value* val,
									 bon_size nelem, bon_type_id type)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return NULL; }
	
	if (val->type != BON_VALUE_AGGREGATE) { return NULL; }
	
	bon_value_agg* agg = &val->u.agg;
	if (agg->type.id != BON_TYPE_ARRAY) { return NULL; }
	
	bon_type_array* arr = agg->type.u.array;
	if (arr->type->id != type)  { return NULL; }
	if (arr->size != nelem)     { return NULL; }
	
	return agg->data; // Win
}
