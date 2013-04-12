//
//  write.c
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).


#include "bon.h"
#include "private.h"
#include "crc32.h"
#include <assert.h>
#include <stdarg.h>       // va_list, va_start, va_arg, va_end
#include <stdlib.h>       // malloc, free, realloc, calloc, ...


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
BON_INLINE void bon_write_to_writer(bon_w_doc* B, const void* data, bon_size n)
{
	if (!B->writer(B->userData, data, n)) {
		B->error = BON_ERR_WRITE_ERROR;
	}
	
	if (B->flags & BON_W_FLAG_CRC) {
		B->crc_inv = crc_update(B->crc_inv, (const uint8_t*)data, n);
	}
}

void bon_w_flush(bon_w_doc* B) {
	if (B->buff_ix > 0) {
		bon_write_to_writer(B, B->buff, B->buff_ix);
		B->buff_ix = 0;
	}
}

void bon_w_raw_flush_buff(bon_w_doc* B, const void* data, bon_size bs) {
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
	bon_w_doc* B = BON_CALLOC_TYPE(1, bon_w_doc);
	B->writer    = writer;
	B->userData  = userData;
	B->crc_inv   = 0xffffffff;
	B->error     = BON_SUCCESS;
	B->flags     = flags;
	
#if 0
	// Slower.
	B->buff      = NULL;
	B->buff_ix   = 0;
	B->buff_size = 0;
#else
	const unsigned BUFF_SIZE = 64*1024;
	B->buff      = malloc(BUFF_SIZE);
	B->buff_ix   = 0;
	B->buff_size = BUFF_SIZE;
#endif
	
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

void bon_w_begin_block_sized(bon_w_doc* B, bon_block_id block_id, bon_size nbytes)
{
	bon_w_ctrl_vlq(B, BON_CTRL_BLOCK_BEGIN, block_id);
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


//------------------------------------------------------------------------------

void bon_w_packegate_type(bon_w_doc* B, bon_type* type);

void bon_w_pack_array_type(bon_w_doc* B, bon_size length, bon_type* element_type)
{
	if (element_type->id == BON_TYPE_UINT8 && length < BON_SHORT_BYTE_ARRAY_COUNT)
	{
		bon_w_raw_uint8(B, BON_SHORT_BYTE_ARRAY(length));
		return;
	}
	
	if (length < BON_SHORT_ARRAY_COUNT)
	{
		bon_w_raw_uint8(B, BON_SHORT_ARRAY(length));
		bon_w_packegate_type(B, element_type);
		return;
	}
	
	bon_w_ctrl_vlq(B, BON_CTRL_ARRAY_VLQ, length);
	bon_w_packegate_type(B, element_type);
}

void bon_w_packegate_type(bon_w_doc* B, bon_type* type)
{
	switch (type->id) {
		case BON_TYPE_ARRAY:
			bon_w_pack_array_type(B, type->u.array->size, type->u.array->type);
			break;
			
			
		case BON_TYPE_STRUCT: {
			bon_type_struct* strct = type->u.strct;
			bon_size n = strct->size;
			
			if (n < BON_SHORT_STRUCT_COUNT) {
				bon_w_raw_uint8(B, BON_SHORT_STRUCT(n));
			}
			else {
				bon_w_ctrl_vlq(B, BON_CTRL_STRUCT_VLQ, n);
			}
			
			for (bon_size ti=0; ti<n; ++ti) {
				bon_kt* kt = strct->kts + ti;
				bon_w_cstring(B, kt->key);
				bon_w_packegate_type(B, &kt->type);
			}
		} break;
			
			
		default:
			// Simple type - write ctrl code:
			bon_w_raw_uint8(B, type->id);
			break;
	}
}

void bon_w_pack(bon_w_doc* B, const void* data, bon_size nbytes, bon_type* type)
{
	// Sanity check:
	bon_size expected_size = bon_aggregate_payload_size(type);
	if (nbytes != expected_size) {
		fprintf(stderr, "bon_w_pack: wrong size. Expected %d, got %d\n", (int)expected_size, (int)nbytes);
		bon_w_set_error(B, BON_ERR_BAD_AGGREGATE_SIZE);
		return;
	}
	
	bon_w_packegate_type(B, type);
	bon_w_raw(B, data, nbytes);
}

void bon_w_pack_fmt(bon_w_doc* B, const void* data, bon_size nbytes,
						  const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bon_type* type = bon_new_type_fmt_ap(&fmt, &ap);
	va_end(ap);
	
	if (type) {
		bon_w_pack(B, data, nbytes, type);
		bon_free_type(type);
	}
}

void bon_w_pack_array(bon_w_doc* B, const void* data, bon_size nbytes,
							 bon_size len, bon_type_id element_t)
{
	bon_w_assert(B, len * bon_type_size(element_t) == nbytes,
					 BON_ERR_BAD_AGGREGATE_SIZE);
	if (B->error) {
		return;
	}
	
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
	
	bon_w_ctrl_vlq(B, BON_CTRL_ARRAY_VLQ, len);
	bon_w_raw_uint8(B, element_t);
	
	bon_w_raw(B, data, nbytes);
}
