//
//  json2bon.c
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#include <stdio.h>
#include <assert.h>
#include <jansson.h>
#include "hashtable.h" // hashtable_iter_serial
#include <bon/bon.h>

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
	
	bon_w_obj_begin(B);
	for (i = 0; i < size; ++i)
	{
		const char* key   = keys[i].key;
		json_t*     value = json_object_get(json, key);
		bon_w_key(B, key);
		if (!write_json(value, B)) {
			success = BON_FALSE;
			if (!ATTEMPT_RECOVERY)
				break;
		}
	}
	bon_w_obj_end(B);
	
	free(keys);	
#else
	const char* key;
	json_t* value;
	
	bon_w_obj_begin(B);
	json_object_foreach(json, key, value) {
		bon_w_key(B, key);
		if (!write_json(value, B)) {
			success = BON_FALSE;
			if (!ATTEMPT_RECOVERY)
				break;
		}
	}
	bon_w_obj_end(B);
#endif
	
	return success;
}


bon_bool write_json(json_t* json, bon_w_doc* B)
{
	if (!json) {
		fprintf(stderr, "write_json got NULL\n");
		return BON_FALSE;
	}
	
	if (bon_w_error(B) != BON_SUCCESS) {
		if (ATTEMPT_RECOVERY) {
			bon_w_set_error(B, BON_SUCCESS);
		}
	}
	
	bon_bool success = BON_TRUE;
	
	switch (json_typeof(json))
	{
		case JSON_OBJECT: {
			return write_obj(json, B);
		} break;
			
			
		case JSON_ARRAY: {
			size_t size = json_array_size(json);
			
			bon_w_list_begin(B);
			for (size_t ix=0; ix<size; ++ix) {
				json_t* elem = json_array_get(json, ix);
				if (!write_json(elem, B)) {
					success = BON_FALSE;
					if (!ATTEMPT_RECOVERY)
						break;
				}
			}
			bon_w_list_end(B);
		} break;
			
			
		case JSON_STRING:
			bon_w_cstring(B, json_string_value(json));
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
	
	return success && (bon_w_error(B) == BON_SUCCESS);
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
	
	bon_w_doc* B = bon_w_new(&bon_file_writer, out, BON_W_FLAG_DEFAULT);
	
	if (!B) {
		return BON_FALSE;
	}
	
	bon_bool success = write_json(json, B);
	
	if (bon_w_close(B) != BON_SUCCESS) {
		success = BON_FALSE;
	}
	
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
