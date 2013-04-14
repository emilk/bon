//
//  write_inline.h
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#ifndef BON_write_inline_h
#define BON_write_inline_h

#include <assert.h>
#include <math.h>     // isfinite
#include "utf.h"      // utf8_check_string

//#define inline

#define BON_INLINE static inline

#ifndef isfinite
BON_INLINE int isfinite(double x) { return x-x == 0.0; }
#endif


BON_INLINE bon_error bon_w_error(bon_w_doc* B) {
	return B->error;
}

//------------------------------------------------------------------------------
// Util functions:

void bon_w_raw_flush_buff(bon_w_doc* B, const void* data, bon_size bs);

BON_INLINE void bon_w_raw(bon_w_doc* B, const void* data, bon_size bs)
{
	if (B->buff_ix + bs < B->buff_size) {
		// Fits in the buffer
		memcpy(B->buff + B->buff_ix, data, bs);
		B->buff_ix += bs;
	} else {
		bon_w_raw_flush_buff(B, data, bs);
	}
}


BON_INLINE void bon_w_raw_uint8(bon_w_doc* B, uint8_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

BON_INLINE void bon_w_raw_uint16(bon_w_doc* B, uint16_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

BON_INLINE void bon_w_raw_uint32(bon_w_doc* B, uint32_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

BON_INLINE void bon_w_raw_uint64(bon_w_doc* B, uint64_t val) {
	bon_w_raw(B, &val, sizeof(val));
}


//------------------------------------------------------------------------------
// VLQ
// See http://en.wikipedia.org/wiki/Variable-length_quantity
// See http://rosettacode.org/wiki/Variable-length_quantity


// Maximum number of bytes to encode a very large number
#define BON_VARINT_MAX_LEN 10

BON_INLINE uint32_t bon_vlq_size(bon_size x)
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
BON_INLINE uint32_t bon_w_vlq_to(uint8_t* out, bon_size x)
{
	uint32_t size = bon_vlq_size(x);
	
	for (uint32_t i = 0; i < size; ++i) {
		out[i] = ((x >> ((size - 1 - i) * 7)) & 0x7f) | 0x80;
	}
	
	out[size-1] &= 0x7f; // Remove last flag
	
	return size;
}

#if 1
BON_INLINE void bon_w_vlq(bon_w_doc* B, bon_size x)
{
	uint8_t buff[BON_VARINT_MAX_LEN];
	uint32_t size = bon_w_vlq_to(buff, x);
	bon_w_raw(B, buff, size);
}
#endif

// Done often, lets do it fast:
BON_INLINE void bon_w_ctrl_vlq(bon_w_doc* B, bon_ctrl ctrl, bon_size x)
{
	if (x < 128) {
		// Common case optimization
		uint8_t buff[2] = { (uint8_t)ctrl, (uint8_t)x };
		bon_w_raw(B, buff, 2);
	} else {
		uint8_t buff[1 + BON_VARINT_MAX_LEN];
		buff[0] = ctrl;
		uint32_t size = bon_w_vlq_to(1 + buff, x);
		bon_w_raw(B, buff, 1 + size);
	}
}

//------------------------------------------------------------------------------
// Public API functions:


/*
 A much faster version of
 bon_w_raw_uint8(B, type_ctrl);
 bon_w_raw(B, &data, sizeof(data));
 */
#define BON_WRITE_QUICKLY(type_ctrl, data)     \
/**/    unsigned char buf[1 + sizeof(data)];   \
/**/    buf[0] = type_ctrl;                    \
/**/    memcpy(buf+1, &data, sizeof(data));    \
/**/    bon_w_raw(B, buf, sizeof(buf));        \


BON_INLINE void bon_w_obj_begin(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_BEGIN);
}

BON_INLINE void bon_w_obj_end(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_END);
}

BON_INLINE void bon_w_key(bon_w_doc* B, const char* utf8) {
	bon_w_cstring(B, utf8);
}

BON_INLINE void bon_w_list_begin(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_BEGIN);
}

BON_INLINE void bon_w_list_end(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_END);
}


BON_INLINE void bon_w_block_ref(bon_w_doc* B, bon_block_id block_id)
{
	if (block_id < BON_SHORT_BLOCK_COUNT) {
		bon_w_raw_uint8(B, BON_SHORT_BLOCK(block_id));
	} else {
		bon_w_ctrl_vlq(B, BON_CTRL_BLOCK_REF, block_id);
	}
}

BON_INLINE void bon_w_nil(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_NIL);
}

BON_INLINE void bon_w_bool(bon_w_doc* B, bon_bool val) {
	if (val) {
		bon_w_raw_uint8(B, BON_CTRL_TRUE);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_FALSE);
	}
}

BON_INLINE void bon_w_string(bon_w_doc* B, const char* utf8, bon_size nbytes) {
	if (nbytes == BON_ZERO_ENDED) {
		nbytes = strlen(utf8);
	}
	
	if ((B->flags & BON_W_FLAG_SKIP_STRING_CHECKS) == 0)
	{
		if (!utf8_check_string(utf8, nbytes)) {
			// Invalid UTF-8.
			bon_w_set_error(B, BON_ERR_NOT_UTF8);
		}
	}
	
	if (nbytes < BON_SHORT_STRING_COUNT) {
		bon_w_raw_uint8(B, BON_SHORT_STRING(nbytes));
	} else {
		bon_w_ctrl_vlq(B, BON_CTRL_STRING_VLQ, nbytes);
	}
	
	bon_w_raw(B, utf8, nbytes);
	bon_w_raw_uint8(B, 0); // Zero-ended
}

BON_INLINE void bon_w_cstring(bon_w_doc* B, const char* utf8)
{
	bon_w_string(B, utf8, BON_ZERO_ENDED);
}

BON_INLINE void bon_w_uint64(bon_w_doc* B, uint64_t u64)
{
	if (u64 < BON_SHORT_POS_INT_COUNT) {
		bon_w_raw_uint8(B, (uint8_t)u64);
	} else if (u64 == (u64&0xff)) {
		uint8_t u8 = (uint8_t)u64;
		BON_WRITE_QUICKLY(BON_CTRL_UINT8, u8);
	} else if (u64 == (u64&0xffff)) {
		uint16_t u16 = (uint16_t)u64;
		BON_WRITE_QUICKLY(BON_CTRL_UINT16, u16);
	} else if (u64 == (u64&0xffffffff)) {
		uint32_t u32 = (uint32_t)u64;
		BON_WRITE_QUICKLY(BON_CTRL_UINT32, u32);
	} else {
		BON_WRITE_QUICKLY(BON_CTRL_UINT64, u64);
	}
}

BON_INLINE void bon_w_sint64(bon_w_doc* B, int64_t s64) {
	if (s64 >= 0) {
		bon_w_uint64(B, (uint64_t)s64);
	} else if (-16 <= s64) {
		bon_w_raw_uint8(B, (uint8_t)s64);
	} else if (-0x80 <= s64 && s64 < 0x80) {
		uint8_t u8 = (uint8_t)s64;
		BON_WRITE_QUICKLY(BON_CTRL_SINT8, u8);
	} else if (-0x8000 <= s64 && s64 < 0x8000) {
		uint16_t u16 = (uint16_t)s64;
		BON_WRITE_QUICKLY(BON_CTRL_SINT16, u16);
	} else if (-0x80000000LL <= s64 && s64 < 0x80000000LL) {
		uint32_t u32 = (uint32_t)s64;
		BON_WRITE_QUICKLY(BON_CTRL_SINT32, u32);
	} else {
		BON_WRITE_QUICKLY(BON_CTRL_SINT64, s64);
	}
}


BON_INLINE void bon_w_float(bon_w_doc* B, float val)
{
#if 1
	// I think this can be optimized to testing just the exponent sign bit.
	int64_t ival = (int64_t)val;
	if (val == (float)ival) {
		bon_w_sint64(B, ival);
		return;
	}
#endif
	
	BON_WRITE_QUICKLY(BON_CTRL_FLOAT, val);
}

BON_INLINE void bon_w_double(bon_w_doc* B, double val)
{
	if (!isfinite(val) || (double)(float)val == val) {
		bon_w_float(B, (float)val);
	} else {
		BON_WRITE_QUICKLY(BON_CTRL_DOUBLE, val);
	}
}


#endif
