//
//  json2bon.c
//  BON
//
//  Created by emilk on 2013-02-04.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include <stdio.h>
#include <assert.h>
#include "jansson.h"
#include "hashtable.h" // hashtable_iter_serial
#include "bon.h"

#define PRESERVE_ORDER 1
#define ATTEMPT_RECOVERY 1

bon_bool write_json(json_t* json, bon_w_doc* B);


struct object_key {
	size_t serial;
	const char *key;
};

static int object_key_compare_serials(const void *key1, const void *key2)
{
	size_t a = ((const struct object_key *)key1)->serial;
	size_t b = ((const struct object_key *)key2)->serial;
	
	return a < b ? -1 : a == b ? 0 : 1;
}

bon_bool write_obj(json_t* json, bon_w_doc* B)
{
	bon_bool success = BON_TRUE;
	
#if PRESERVE_ORDER	
	size_t size = json_object_size(json);
	struct object_key* keys = malloc(size * sizeof(struct object_key));
	
	void* iter = json_object_iter((json_t *)json);
	
	size_t i = 0;
	while(iter)
	{
		keys[i].serial = hashtable_iter_serial(iter);
		keys[i].key    = json_object_iter_key(iter);
		iter = json_object_iter_next((json_t *)json, iter);
		++i;
	}
	assert(i == size);
		
	qsort(keys, size, sizeof(struct object_key), object_key_compare_serials);
	
	bon_w_begin_obj(B);
	for (i = 0; i < size; ++i)
	{
		const char* key   = keys[i].key;
		json_t*     value = json_object_get(json, key);
		bon_w_key(B, key, BON_ZERO_ENDED);
		if (!write_json(value, B)) {
			success = BON_FALSE;
			if (!ATTEMPT_RECOVERY)
				break;
		}
	}
	bon_w_end_obj(B);
	
	free(keys);	
#else
	const char* key;
	json_t* value;
	
	bon_w_begin_obj(B);
	json_object_foreach(json, key, value) {
		bon_w_key(B, key, BON_ZERO_ENDED);
		if (!write_json(value, B)) {
			success = BON_FALSE;
			if (!ATTEMPT_RECOVERY)
				break;
		}
	}
	bon_w_end_obj(B);
#endif
	
	return success;
}


bon_bool write_json(json_t* json, bon_w_doc* B)
{
	if (!json) {
		fprintf(stderr, "write_json got NULL\n");
		return BON_FALSE;
	}
	
	if (B->error) {
		if (ATTEMPT_RECOVERY)
			B->error = 0;
	}
	
	bon_bool success = BON_TRUE;
	
	switch (json_typeof(json))
	{
		case JSON_OBJECT: {
			return write_obj(json, B);
		} break;
			
			
		case JSON_ARRAY: {
			size_t size = json_array_size(json);
			
			bon_w_begin_list(B);
			for (size_t ix=0; ix<size; ++ix) {
				json_t* elem = json_array_get(json, ix);
				if (!write_json(elem, B)) {
					success = BON_FALSE;
					if (!ATTEMPT_RECOVERY)
						break;
				}
			}
			bon_w_end_list(B);
		} break;
			
			
		case JSON_STRING:
			bon_w_string(B, json_string_value(json), BON_ZERO_ENDED);
			break;
			
			
		case JSON_INTEGER:
			bon_w_sint64(B, json_integer_value(json));
			break;
			
			
		case JSON_REAL:
			bon_w_double(B, json_real_value(json));
			break;
			
			
		case JSON_TRUE:
			bon_w_bool(B, BON_TRUE);
			break;
			
			
		case JSON_FALSE:
			bon_w_bool(B, BON_FALSE);
			break;
			
			
		case JSON_NULL:
			bon_w_nil(B);
			break;
			
			
		default:
			fprintf(stderr, "Bad JSON object\n");
			return BON_FALSE;
	}
	
	return success && (B->error == BON_SUCCESS);
}

bon_bool handle(json_t* json, json_error_t* err, FILE* out) {
	if (!json) {
		if (err->text[0] != '\0') {
			fprintf(stderr, "%s, line: %d, col: %d\n", err->text, err->line, err->column);
			return BON_FALSE;
		}
		
		fprintf(stderr, "Unknown JSON parse error at line: %d, col: %d\n", err->line, err->column);
		return BON_FALSE;
	}
	
	bon_w_doc* B = bon_w_new_doc(&bon_file_writer, out, BON_FLAG_DEFAULT);
	
	if (!B) {
		return BON_FALSE;
	}
	
	bon_w_header(B);	
	bon_bool success = write_json(json, B);
	bon_w_footer(B);
	if (B->error)
		success = BON_FALSE;
	bon_w_close_doc(B);
	
	return success;
}


int main(int argc, const char * argv[])
{
	bon_bool didParseFile = BON_FALSE;
	FILE* out = stdout;
	size_t flags = JSON_DECODE_ANY;
	/* Flags:
	 JSON_REJECT_DUPLICATES
	 JSON_DECODE_ANY
	 JSON_DISABLE_EOF_CHECK
	 */
		
	for (int i=1; i<argc; ++i) {
		if (argv[i][0] == '-') {
			
		} else {
			// File-name
			json_error_t err;
			json_t* json = json_load_file(argv[i], flags, &err);
			if (!handle(json, &err, out)) {
				return EXIT_FAILURE;
			}
			didParseFile = BON_TRUE;
		}
	}
	
	if (!didParseFile) {
		json_error_t err;
		json_t* json = json_loadf(stdin, flags, &err);
		if (!handle(json, &err, out)) {
			return EXIT_FAILURE;
		}
	}
	
	return EXIT_SUCCESS;
}
