//
//  write.c
//  BON
//
//  Created by emilk on 2013-04-07.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//


#include "bon.h"
#include "bon_private.h"
#include "crc32.h"
#include <assert.h>
#include <math.h>         // isfinite
#include <stdarg.h>       // va_list, va_start, va_arg, va_end
#include <stdlib.h>       // malloc, free, realloc, calloc, ...
#include <string.h>       // memcpy, memset


//------------------------------------------------------------------------------


bon_bool bon_vec_writer(void* userData, const void* data, uint64_t nbytes) {
	bon_byte_vec* vec = (bon_byte_vec*)userData;
	bon_size oldSize = vec->size;
	BON_VECTOR_EXPAND(*vec, uint8_t, nbytes);
	if (vec->data) {
		memcpy(vec->data + oldSize, data, nbytes);
		return BON_TRUE;
	} else {
		// Alloc fail
		return BON_FALSE;
	}
}


bon_bool bon_file_writer(void* user, const void* data, uint64_t nbytes)
{
	FILE* fp = (FILE*)user;
	fwrite(data, 1, nbytes, fp);
	return !ferror(fp);
}


//------------------------------------------------------------------------------


bon_error bon_w_error(bon_w_doc* B) {
	return B->error;
}

void bon_w_set_error(bon_w_doc* B, bon_error err)
{
	bon_onError(bon_err_str(err));
	
	if (B->error == BON_SUCCESS) {
		B->error = err;
	}
}


void bon_w_assert(bon_w_doc* B, bon_bool statement, bon_error onFail)
{
	if (!statement) {
		bon_w_set_error(B, onFail);
	}
}



//------------------------------------------------------------------------------
// Writing

// Bypass buffer
void bon_write_to_writer(bon_w_doc* B, const void* data, bon_size n)
{
	if (!B->writer(B->userData, data, n)) {
		B->error = BON_ERR_WRITE_ERROR;
	}
	
	if (B->flags & BON_W_FLAG_CRC) {
		B->crc_inv = crc_update(B->crc_inv, data, n);
	}
}

void bon_w_flush(bon_w_doc* B) {
	if (B->buff_ix > 0) {
		bon_write_to_writer(B, B->buff, B->buff_ix);
		B->buff_ix = 0;
	}
}

void bon_w_raw(bon_w_doc* B, const void* data, bon_size bs) {
	if (!B->buff) {
		// Unbuffered
		bon_write_to_writer(B, data, bs);
		return;
	}
	
	const bon_size BIG_CHUNK = 1024;
	
	if (bs >= BIG_CHUNK) {
		/*
		 The power of the buffer is to mitigate many small writes (death by many small cuts).
		 A big chunk like this can be sent right to the writer.
		 */
		bon_w_flush(B);
		bon_write_to_writer(B, data, bs);
		return;
	}
	
	if (B->buff_ix + bs < B->buff_size) {
		// Fits in the buffer
		memcpy(B->buff + B->buff_ix, data, bs);
		B->buff_ix += bs;
	} else {
		bon_w_flush(B);
		
		if (bs < B->buff_size) {
			// Now it fits
			memcpy(B->buff, data, bs);
			B->buff_ix = bs;
		} else {
			// Too large for buffer - write directly:
			bon_write_to_writer(B, data, bs);
		}
	}
}

void bon_w_raw_uint8(bon_w_doc* B, uint8_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

void bon_w_raw_uint16(bon_w_doc* B, uint16_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

void bon_w_raw_uint32(bon_w_doc* B, uint32_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

void bon_w_raw_uint64(bon_w_doc* B, uint64_t val) {
	bon_w_raw(B, &val, sizeof(val));
}



//------------------------------------------------------------------------------
// VarInt - standard VLQ
// See http://en.wikipedia.org/wiki/Variable-length_quantity
// See http://rosettacode.org/wiki/Variable-length_quantity

// Maximum number of bytes to encode a very large number
#define BON_VARINT_MAX_LEN 10

uint32_t bon_vlq_size(bon_size x)
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
	uint32_t size = bon_vlq_size(x);
	
	for (uint32_t i = 0; i < size; ++i) {
		out[i] = ((x >> ((size - 1 - i) * 7)) & 0x7f) | 0x80;
	}
	
	out[size-1] &= 0x7f; // Remove last flag
	
	return size;
}

uint32_t bon_w_vlq(bon_w_doc* B, bon_size x)
{
	uint8_t vals[BON_VARINT_MAX_LEN];
	uint32_t size = bon_w_vlq_to(vals, x);
	bon_w_raw(B, vals, size);
	return size;
}



//------------------------------------------------------------------------------


void bon_w_header(bon_w_doc* B)
{
	const uint8_t header[4] = {'B', 'O', 'N', '0'};
	bon_w_raw(B, header, 4);
}

void bon_w_footer(bon_w_doc* B)
{
	if (B->flags & BON_W_FLAG_CRC)
	{
		// Add contribution of buffered data:
		B->crc_inv = crc_update(B->crc_inv, B->buff, B->buff_ix);
		
		uint32_t crc = B->crc_inv ^ 0xffffffff;
		uint32_t crc_le = uint32_to_le(crc);
		
		bon_w_raw_uint8 (B, BON_CTRL_FOOTER_CRC);
		bon_w_raw_uint32(B, crc_le);
		bon_w_raw_uint8 (B, BON_CTRL_FOOTER_CRC);
	}
	else
	{
		bon_w_raw_uint8 (B, BON_CTRL_FOOTER);
	}
}

bon_w_doc* bon_w_new_doc(bon_w_writer_t writer, void* userData, bon_w_flags flags)
{
	bon_w_doc* B = BON_ALLOC_TYPE(1, bon_w_doc);
	B->writer    = writer;
	B->userData  = userData;
	B->crc_inv   = 0xffffffff;
	B->error     = BON_SUCCESS;
	B->flags     = flags;
	
	const unsigned BUFF_SIZE = 64*1024;
	B->buff      = malloc(BUFF_SIZE);
	B->buff_ix   = 0;
	B->buff_size = BUFF_SIZE;
	
	if ((B->flags & BON_W_FLAG_SKIP_HEADER_FOOTER) == 0) {
		bon_w_header(B);
	}
	
	return B;
}

bon_error bon_w_close_doc(bon_w_doc* B)
{
	if ((B->flags & BON_W_FLAG_SKIP_HEADER_FOOTER) == 0) {
		bon_w_footer(B);
	}
	bon_w_flush(B);
	bon_error err = B->error;
	free(B->buff);
	free(B);
	return err;
}

void bon_w_block_ref(bon_w_doc* B, bon_block_id block_id)
{
	if ((B->flags & BON_W_FLAG_NO_COMPRESS) == 0 && block_id < BON_SHORT_BLOCK_COUNT) {
		bon_w_raw_uint8(B, BON_SHORT_BLOCK(block_id));
	} else {
		bon_w_raw_uint8(B, BON_CTRL_BLOCK_REF);
		bon_w_vlq(B, block_id);
	}
}

void bon_w_begin_block_sized(bon_w_doc* B, bon_block_id block_id, bon_size nbytes)
{
	bon_w_raw_uint8(B, BON_CTRL_BLOCK_BEGIN);
	bon_w_vlq(B, block_id);
	bon_w_vlq(B, nbytes);
}

void bon_w_begin_block(bon_w_doc* B, bon_block_id block_id)
{
	bon_w_begin_block_sized(B, block_id, 0);
}

void bon_w_end_block(bon_w_doc* B)
{
	bon_w_raw_uint8(B, BON_CTRL_BLOCK_END);
}

void bon_w_block(bon_w_doc* B, bon_block_id block_id, const void* data, bon_size nbytes)
{
	bon_w_begin_block_sized(B, block_id, nbytes);
	bon_w_raw(B, data, nbytes);
	bon_w_end_block(B);
}


//------------------------------------------------------------------------------
// Value writing

void bon_w_begin_obj(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_BEGIN);
}

void bon_w_end_obj(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_END);
}

void bon_w_key(bon_w_doc* B, const char* utf8) {
	bon_w_cstring(B, utf8);
}

void bon_w_begin_list(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_BEGIN);
}

void bon_w_end_list(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_END);
}

void bon_w_nil(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_NIL);
}

void bon_w_bool(bon_w_doc* B, bon_bool val) {
	if (val) {
		bon_w_raw_uint8(B, BON_CTRL_TRUE);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_FALSE);
	}
}

void bon_w_string(bon_w_doc* B, const char* utf8, bon_size nbytes) {
	if (nbytes == BON_ZERO_ENDED) {
		nbytes = strlen(utf8);
	}
	
	if (B->flags & BON_W_FLAG_NO_COMPRESS || nbytes >= BON_SHORT_STRING_COUNT) {
		bon_w_raw_uint8(B, BON_CTRL_STRING_VLQ);
		bon_w_vlq(B, nbytes);
	} else {
		bon_w_raw_uint8(B, BON_SHORT_STRING(nbytes));
	}
	
	bon_w_raw(B, utf8, nbytes);
	bon_w_raw_uint8(B, 0); // Zero-ended
}

void bon_w_cstring(bon_w_doc* B, const char* utf8)
{
	bon_w_string(B, utf8, BON_ZERO_ENDED);
}

void bon_w_uint64(bon_w_doc* B, uint64_t val)
{
	if (val < BON_SHORT_POS_INT_COUNT && !(B->flags & BON_W_FLAG_NO_COMPRESS)) {
		bon_w_raw_uint8(B, (uint8_t)val);
	} else if (val == (val&0xff)) {
		bon_w_raw_uint8(B, BON_CTRL_UINT8);
		bon_w_raw_uint8(B, (uint8_t)val);
	} else if (val == (val&0xffff)) {
		bon_w_raw_uint8(B, BON_CTRL_UINT16);
		bon_w_raw_uint16(B, (uint16_t)val);
	} else if (val == (val&0xffffffff)) {
		bon_w_raw_uint8(B, BON_CTRL_UINT32);
		bon_w_raw_uint32(B, (uint32_t)val);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_UINT64);
		bon_w_raw_uint64(B, (uint64_t)val);
	}
}

void bon_w_sint64(bon_w_doc* B, int64_t val) {
	if (val >= 0) {
		bon_w_uint64(B, (uint64_t)val);
	} else if (-16 <= val && !(B->flags & BON_W_FLAG_NO_COMPRESS)) {
		bon_w_raw_uint8(B, (uint8_t)val);
	} else if (-0x80 <= val && val < 0x80) {
		bon_w_raw_uint8(B, BON_CTRL_SINT8);
		bon_w_raw_uint8(B, (uint8_t)val);
	} else if (-0x8000 <= val && val < 0x8000) {
		bon_w_raw_uint8(B, BON_CTRL_SINT16);
		bon_w_raw_uint16(B, (uint16_t)val);
	} else if (-0x80000000LL <= val && val < 0x80000000LL) {
		bon_w_raw_uint8(B, BON_CTRL_SINT32);
		bon_w_raw_uint32(B, (uint32_t)val);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_SINT64);
		bon_w_raw_uint64(B, (uint64_t)val);
	}
}

void bon_w_float(bon_w_doc* B, float val)
{
#if 1
	int64_t ival = (int64_t)val;
	if (val == (float)ival) {
		bon_w_sint64(B, ival);
		return;
	}
#endif
	
	bon_w_raw_uint8(B, BON_CTRL_FLOAT32);
	bon_w_raw(B, &val, 4);
}

void bon_w_double(bon_w_doc* B, double val)
{
	if (!isfinite(val) || (double)(float)val == val) {
		bon_w_float(B, (float)val);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_FLOAT64);
		bon_w_raw(B, &val, 8);
	}
}

void bon_w_aggregate_type(bon_w_doc* B, bon_type* type);

void bon_w_array_type(bon_w_doc* B, bon_size length, bon_type* element_type)
{
	if ((B->flags & BON_W_FLAG_NO_COMPRESS) == 0)
	{
		if (element_type->id == BON_TYPE_UINT8 && length < BON_SHORT_BYTE_ARRAY_COUNT)
		{
			bon_w_raw_uint8(B, BON_SHORT_BYTE_ARRAY(length));
			return;
		}
		
		if (length < BON_SHORT_ARRAY_COUNT)
		{
			bon_w_raw_uint8(B, BON_SHORT_ARRAY(length));
			bon_w_aggregate_type(B, element_type);
			return;
		}
	}
	
	bon_w_raw_uint8(B, BON_CTRL_ARRAY_VLQ);
	bon_w_vlq(B, length);
	bon_w_aggregate_type(B, element_type);
}

void bon_w_aggregate_type(bon_w_doc* B, bon_type* type)
{
	switch (type->id) {
		case BON_TYPE_ARRAY:
			bon_w_array_type(B, type->u.array->size, type->u.array->type);
			break;
			
			
		case BON_TYPE_STRUCT: {
			bon_type_struct* strct = type->u.strct;
			bon_size n = strct->size;
			
			if ((B->flags & BON_W_FLAG_NO_COMPRESS) == 0 &&
				 n < BON_SHORT_STRUCT_COUNT)
			{
				bon_w_raw_uint8(B, BON_SHORT_STRUCT(n));
			}
			else {
				bon_w_raw_uint8(B, BON_CTRL_STRUCT_VLQ);
				bon_w_vlq(B, n);
			}
			
			for (bon_size ti=0; ti<n; ++ti) {
				bon_kt* kt = strct->kts + ti;
				bon_w_cstring(B, kt->key);
				bon_w_aggregate_type(B, &kt->type);
			}
		} break;
			
			
		default:
			// Simple type - write ctrl code:
			bon_w_raw_uint8(B, type->id);
			break;
	}
}

void bon_w_aggr(bon_w_doc* B, const void* data, bon_size nbytes, bon_type* type)
{
	// Sanity check:
	bon_size expected_size = bon_aggregate_payload_size(type);
	if (nbytes != expected_size) {
		fprintf(stderr, "bon_w_aggr: wrong size. Expected %d, got %d\n", (int)expected_size, (int)nbytes);
		bon_w_set_error(B, BON_ERR_BAD_AGGREGATE_SIZE);
		return;
	}
	
	bon_w_aggregate_type(B, type);
	bon_w_raw(B, data, nbytes);
}

void bon_w_aggr_fmt(bon_w_doc* B, const void* data, bon_size nbytes,
						  const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bon_type* type = bon_new_type_fmt_ap(&fmt, &ap);
	va_end(ap);
	
	if (type) {
		bon_w_aggr(B, data, nbytes, type);
		bon_free_type(type);
	}
}

void bon_w_array(bon_w_doc* B, bon_size len, bon_type_id element_t,
					  const void* data, bon_size nbytes)
{
	bon_w_assert(B, len * bon_type_size(element_t) == nbytes,
					 BON_ERR_BAD_AGGREGATE_SIZE);
	if (B->error) {
		return;
	}
	
	if ((B->flags & BON_W_FLAG_NO_COMPRESS) == 0)
	{
		if (element_t == BON_TYPE_UINT8 && len < BON_SHORT_BYTE_ARRAY_COUNT)
		{
			bon_w_raw_uint8(B, BON_SHORT_BYTE_ARRAY(len));
			bon_w_raw(B, data, nbytes);
			return;
		}
		
		if (len < BON_SHORT_ARRAY_COUNT)
		{
			bon_w_raw_uint8(B, BON_SHORT_ARRAY(len));
			bon_w_raw_uint8(B, element_t);
			bon_w_raw(B, data, nbytes);
			return;
		}
	}
	
	bon_w_raw_uint8(B, BON_CTRL_ARRAY_VLQ);
	bon_w_vlq(B, len);
	bon_w_raw_uint8(B, element_t);
	
	bon_w_raw(B, data, nbytes);
}
