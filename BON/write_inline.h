//
//  write_inline.h
//  BON
//
//  Created by emilk on 2013-04-10.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#ifndef BON_write_inline_h
#define BON_write_inline_h

#include <assert.h>
#include "crc32.h"

//#define inline

//------------------------------------------------------------------------------
// Util functions:

void bon_w_raw_flush_buff(bon_w_doc* B, const void* data, bon_size bs);

static inline void bon_w_raw(bon_w_doc* B, const void* data, bon_size bs)
{
	if (B->buff_ix + bs < B->buff_size) {
		// Fits in the buffer
		memcpy(B->buff + B->buff_ix, data, bs);
		B->buff_ix += bs;
	} else {
		bon_w_raw_flush_buff(B, data, bs);
	}
}


static inline void bon_w_raw_uint8(bon_w_doc* B, uint8_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

static inline void bon_w_raw_uint16(bon_w_doc* B, uint16_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

static inline void bon_w_raw_uint32(bon_w_doc* B, uint32_t val) {
	bon_w_raw(B, &val, sizeof(val));
}

static inline void bon_w_raw_uint64(bon_w_doc* B, uint64_t val) {
	bon_w_raw(B, &val, sizeof(val));
}


//------------------------------------------------------------------------------
// VLQ
// See http://en.wikipedia.org/wiki/Variable-length_quantity
// See http://rosettacode.org/wiki/Variable-length_quantity


// Maximum number of bytes to encode a very large number
#define BON_VARINT_MAX_LEN 10

static inline uint32_t bon_vlq_size(bon_size x)
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
static inline uint32_t bon_w_vlq_to(uint8_t* out, bon_size x)
{
	uint32_t size = bon_vlq_size(x);
	
	for (uint32_t i = 0; i < size; ++i) {
		out[i] = ((x >> ((size - 1 - i) * 7)) & 0x7f) | 0x80;
	}
	
	out[size-1] &= 0x7f; // Remove last flag
	
	return size;
}

static inline uint32_t bon_w_vlq(bon_w_doc* B, bon_size x)
{
	uint8_t vals[BON_VARINT_MAX_LEN];
	uint32_t size = bon_w_vlq_to(vals, x);
	bon_w_raw(B, vals, size);
	return size;
}

//------------------------------------------------------------------------------
// Public API functions:


/*
 A much faster version of
 bon_w_raw_uint8(B, type_ctrl);
 bon_w_raw(B, &val, sizeof(val));
 */
#define BON_WRITE_QUICKLY(type_ctrl)           \
/**/    unsigned char buf[1 + sizeof(val)];    \
/**/    buf[0] = type_ctrl;                    \
/**/    memcpy(buf+1, &val, sizeof(val));      \
/**/    bon_w_raw(B, buf, sizeof(buf));        \


static inline void bon_w_begin_obj(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_BEGIN);
}

static inline void bon_w_end_obj(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_OBJ_END);
}

static inline void bon_w_key(bon_w_doc* B, const char* utf8) {
	bon_w_cstring(B, utf8);
}

static inline void bon_w_begin_list(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_BEGIN);
}

static inline void bon_w_end_list(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_LIST_END);
}


static inline void bon_w_block_ref(bon_w_doc* B, bon_block_id block_id)
{
	if (block_id < BON_SHORT_BLOCK_COUNT) {
		bon_w_raw_uint8(B, BON_SHORT_BLOCK(block_id));
	} else {
		bon_w_raw_uint8(B, BON_CTRL_BLOCK_REF);
		bon_w_vlq(B, block_id);
	}
}

static inline void bon_w_nil(bon_w_doc* B) {
	bon_w_raw_uint8(B, BON_CTRL_NIL);
}

static inline void bon_w_bool(bon_w_doc* B, bon_bool val) {
	if (val) {
		bon_w_raw_uint8(B, BON_CTRL_TRUE);
	} else {
		bon_w_raw_uint8(B, BON_CTRL_FALSE);
	}
}

static inline void bon_w_string(bon_w_doc* B, const char* utf8, bon_size nbytes) {
	if (nbytes == BON_ZERO_ENDED) {
		nbytes = strlen(utf8);
	}
	
	if (nbytes < BON_SHORT_STRING_COUNT) {
		bon_w_raw_uint8(B, BON_SHORT_STRING(nbytes));
	} else {
		bon_w_raw_uint8(B, BON_CTRL_STRING_VLQ);
		bon_w_vlq(B, nbytes);
	}
	
	bon_w_raw(B, utf8, nbytes);
	bon_w_raw_uint8(B, 0); // Zero-ended
}

static inline void bon_w_cstring(bon_w_doc* B, const char* utf8)
{
	bon_w_string(B, utf8, BON_ZERO_ENDED);
}

static inline void bon_w_uint64(bon_w_doc* B, uint64_t val)
{
	if (val < BON_SHORT_POS_INT_COUNT) {
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

static inline void bon_w_sint64(bon_w_doc* B, int64_t val) {
	if (val >= 0) {
		bon_w_uint64(B, (uint64_t)val);
	} else if (-16 <= val) {
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

static inline void bon_w_double(bon_w_doc* B, double val)
{
	if (!isfinite(val) || (double)(float)val == val) {
		bon_w_float(B, (float)val);
	} else {
		BON_WRITE_QUICKLY(BON_CTRL_DOUBLE);
	}
}


static inline void bon_w_float(bon_w_doc* B, float val)
{
#if 1
	// I think this can be optimized to testing just the exponent sign bit.
	int64_t ival = (int64_t)val;
	if (val == (float)ival) {
		bon_w_sint64(B, ival);
		return;
	}
#endif
	
	BON_WRITE_QUICKLY(BON_CTRL_FLOAT);
}


#endif