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

// For printf:ing int64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


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

json_t* agg2json(const bon_type* aggType, bon_reader* br)
{
	uint8_t id = aggType->id;
	
	switch (id) {
		case BON_TYPE_ARRAY: {
			bon_type_array* arr = aggType->u.array;
			json_t* jarray = json_array();
			for (uint64_t ti=0; ti<arr->size; ++ti) {
				json_t* elem = agg2json(arr->type, br);
				json_array_append_new( jarray, elem );
			}
			return jarray;
		}
			
		case BON_CTRL_STRUCT_VLQ: {
			bon_type_struct* strct = aggType->u.strct;
			json_t* jobj = json_object();
			for (uint64_t ti=0; ti<strct->size; ++ti) {
				const char* key  = strct->keys[ti];
				json_t*     elem = agg2json(strct->types[ti], br);
				json_object_set_new( jobj, key, elem );
			}
			return jobj;
		}
			
			
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:  case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:  case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:  case BON_CTRL_SINT64_BE: {
			int64_t s64 = br_read_sint64(br, id);
			return json_integer( s64 );
		}
			
			
		case BON_CTRL_UINT8:
		case BON_CTRL_UINT16_LE:  case BON_CTRL_UINT16_BE:
		case BON_CTRL_UINT32_LE:  case BON_CTRL_UINT32_BE:
		case BON_CTRL_UINT64_LE:  case BON_CTRL_UINT64_BE: {
			return uint64_2_json( br_read_uint64(br, id) );
		}
			
			
		case BON_CTRL_FLOAT32_LE:  case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:  case BON_CTRL_FLOAT64_BE: {
			double dbl = br_read_double(br, id);
			return json_real(dbl);
		}
			
			
		default: {
			fprintf(stderr, "Unknown aggregate type\n");
			return NULL;
		}
	}
	
}


json_t* bon2json(const bon_r_doc* bon, const bon_value* v)
{
	switch (v->type)
	{
		case BON_VALUE_NIL:
			return json_null();
			
		case BON_VALUE_BOOL:
			return json_boolean( v->u.boolean );
			
			
		case BON_VALUE_UINT64:
			return uint64_2_json( v->u.u64 );
			
			
		case BON_VALUE_SINT64:
			return json_integer( v->u.s64 );
			
			
		case BON_VALUE_DOUBLE:
			return json_real( v->u.dbl );
			
			
		case BON_VALUE_STRING:
			return json_string( (const char*)v->u.str.ptr );
			
			
		case BON_VALUE_LIST: {
			json_t* jarray = json_array();
			const bon_values* vals = &v->u.list.values;
			bon_size size = vals->size;
			for (bon_size i=0; i<size; ++i) {
				json_t* elem = bon2json( bon, &vals->data[i] );
				json_array_append_new( jarray, elem );
			}
			return jarray;
		}
			
			
		case BON_VALUE_AGGREGATE: {
			size_t byteSize = bon_aggregate_payload_size(&v->u.agg.type);
			bon_reader br = { v->u.agg.data, byteSize, 0, 0 };
			json_t* val = agg2json(&v->u.agg.type, &br);
			if (br.nbytes != 0) {
				// We should have consumed all bytes.
				fprintf(stderr, "Bad aggregate\n");
				return NULL;
			}
			return val;
		}
			
			
		case BON_VALUE_OBJ: {
			json_t* jobj = json_object();
			
			const bon_kvs* kvs = &v->u.obj.kvs;
			bon_size size = kvs->size;
			
			for (bon_size i=0; i<size; ++i) {
				const bon_kv* kv = &kvs->data[i];
				if (kv->key.type != BON_VALUE_STRING) {
					fprintf(stderr, "BON object has non-string key\n");
					continue;
				}
				
				json_t* val = bon2json( bon, &kv->val );
				json_object_set_new(jobj, (const char*)kv->key.u.str.ptr, val);
			}
			
			return jobj;
		}
			
			
		case BON_VALUE_BLOCK_REF: {
			const bon_value* val = bon_r_get_block( bon, v->u.block_id );
			if (val) {
				return bon2json(bon, val);
			} else {
				fprintf(stderr, "Missing block, id %"PRIu64, v->u.block_id );
				return NULL;
			}
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
	
	bon_r_doc* bon = bon_r_open(data, size);	
	json_t* json = bon2json(bon, bon_r_root(bon));
	
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
	
	bon_r_close(bon);
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
		// TODO
		/*
		json_error_t err;
		json_t* json = json_loadf(stdin, flags, &err);
		handle(json, &err, out);
		 */
	}
	
	return EXIT_SUCCESS;
}

