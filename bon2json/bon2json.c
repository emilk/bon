//
//  main.c
//  bon2json
//
//  Created by emilk on 2013-02-04.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include <stdio.h>
#include "jansson.h"
#include "bon.h"

// File size (stat):
#include <sys/stat.h>
#include <sys/types.h>


uint8_t* readFile(size_t* out_size, const char* path)
{
	*out_size = 0;
	
	struct stat info;
	if (stat(path, &info) != 0) {
		fprintf(stderr, "Failed to stat file %s\n", path);
		return NULL;
	}
	
	size_t fileSize = (size_t)info.st_size;
	
	uint8_t* data = (uint8_t*)malloc(fileSize);
	
	FILE* fp = fopen(path, "rb");
	
	size_t blocks_read = fread(data, fileSize, 1, fp);
	if (blocks_read != 1) {
		fprintf(stderr, "Failed to read file %s\n", path);
		return NULL ;
	}
	
	fclose(fp);
	
	*out_size = fileSize;
	return data;
}


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

bon_bool handle_file(const char* path, size_t flags, FILE* out)
{
	size_t size;
	const uint8_t* data = readFile(&size, path);
	
	if (!data) { return BON_FALSE; }
	
	bon_r_doc* B = bon_r_open(data, size, BON_R_FLAG_DEFAULT);
	
	if (bon_r_error(B)) {
		fprintf(stderr, "Faield to parse BON file: %s\n", bon_err_str(bon_r_error(B)));
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
		}
	}
	
	if (!didParseFile) {
		// TODO: read from stdin
		/*
		json_error_t err;
		json_t* json = json_loadf(stdin, flags, &err);
		handle(json, &err, out);
		 */
	}
	
	return EXIT_SUCCESS;
}

