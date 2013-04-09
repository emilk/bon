//
//  main.c
//  bon2json
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#include <stdio.h>
#include "jansson.h"
#include "bon.h"


json_t* uint64_2_json(uint64_t u64)
{
	if (u64 <= 0x7fffffffffffffffULL) {
		return json_integer( (int64_t)u64 );
	} else {
		fprintf(stderr, "Uint64 too large for jansson - converting to double");
		return json_real( (double)u64 );
	}
}

json_t* bon2json(bon_r_doc* B, bon_value* v)
{
	switch (bon_r_value_type(B, v))
	{
		case BON_LOGICAL_NIL:
			return json_null();
			
		case BON_LOGICAL_BOOL:
			return json_boolean( bon_r_bool(B, v) );
			
			
		case BON_LOGICAL_UINT:
			return uint64_2_json( bon_r_uint(B, v) );
			
			
		case BON_LOGICAL_SINT:
			return json_integer( bon_r_int(B, v) );
			
			
		case BON_LOGICAL_DOUBLE:
			return json_real( bon_r_double(B, v) );
			
			
		case BON_LOGICAL_STRING:
			return json_string( bon_r_cstr(B, v) );
			
			
		case BON_LOGICAL_LIST: {
			json_t* jarray = json_array();
			bon_size size = bon_r_list_size(B, v);
			for (bon_size ix=0; ix<size; ++ix) {
				json_t* elem = bon2json( B, bon_r_list_elem(B, v, ix) );
				json_array_append_new( jarray, elem );
			}
			return jarray;
		}
			
		case BON_LOGICAL_OBJECT: {
			json_t* jobj = json_object();
			
			bon_size size = bon_r_obj_size(B, v);
			
			for (bon_size ix=0; ix<size; ++ix) {
				const char*  bkey = bon_r_obj_key(B, v, ix);
				bon_value*   bval = bon_r_obj_value(B, v, ix);
				json_t* val = bon2json(B, bval);
				json_object_set_new(jobj, bkey, val);
			}
			
			return jobj;
		}
			
		default:
			fprintf(stderr, "Unknown BON type\n");
			return NULL;
	}
}

bon_bool handle_bon(const uint8_t* data, size_t size, size_t flags, FILE* out)
{
	if (!data) { return BON_FALSE; }
	
	bon_r_doc* B = bon_r_open(data, size, BON_R_FLAG_DEFAULT);
	
	if (bon_r_error(B)) {
		fprintf(stderr, "Failed to parse BON file: %s\n", bon_err_str(bon_r_error(B)));
		bon_r_close(B);
		return BON_FALSE;
	}
	
	json_t* json = bon2json(B, bon_r_root(B));
	
	if (json) {
		if (json_dumpf(json, out, flags) != 0) {
			fprintf(stderr, "json_dumpf failed\n");
			return BON_FALSE;
		}
		
		if (out == stdout)
			fprintf(stdout, "\n");
		
		return BON_TRUE;
	} else {
		return BON_FALSE;
	}
	
	bon_r_close(B);
}

bon_bool handle_file(const char* path, size_t flags, FILE* out)
{
	bon_size size;
	uint8_t* data = bon_read_file(&size, path);
	bon_bool win = handle_bon(data, size, flags, out);
	free(data);
	return win;
}


uint8_t* readEntireFile(size_t* outSize, FILE* fp)
{
	uint8_t*  buff       = NULL;
	size_t    chunkSize  = 1024*128;
	size_t    nRead      = 0;
	
	while (!feof(fp))
	{
		buff = realloc(buff, nRead + chunkSize);
		
		size_t n = fread(buff + nRead, 1, chunkSize, fp);
		nRead += n;
		
		chunkSize *= 2; // Read progressively larger chunks
	}
	
	*outSize = nRead;
	return buff;
}


int main(int argc, const char * argv[])
{
	bon_bool didParseFile = BON_FALSE;
	FILE* out = stdout;
	size_t flags = 0;
	/* Flags:
	 JSON_INDENT(n)
	 JSON_COMPACT
	 JSON_ENSURE_ASCII
	 JSON_SORT_KEYS
	 JSON_PRESERVE_ORDER
	 JSON_ENCODE_ANY
	 JSON_ESCAPE_SLASH
	 */
	
	flags |= JSON_INDENT(4);
	flags |= JSON_PRESERVE_ORDER;
	flags |= JSON_ESCAPE_SLASH;   // Security
	
	flags |= JSON_ENCODE_ANY; // Forgiving
		
	
	for (int i=1; i<argc; ++i) {
		if (argv[i][0] == '-') {
			
		} else {
			if (!handle_file(argv[i], flags, out))
				return EXIT_FAILURE;
			
			didParseFile = BON_TRUE;
		}
	}
	
	if (!didParseFile) {
		// Read from stdin to an expanding buffer:
		FILE* binary_stdin = freopen(NULL, "rb", stdin);
		size_t size;
		uint8_t* buff = readEntireFile(&size, binary_stdin);
		
		if (!handle_bon(buff, size, flags, out)) {
			return EXIT_FAILURE;
		}
	}
	
	return EXIT_SUCCESS;
}

