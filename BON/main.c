//
//  main.cpp
//  BON
//
//  Created by emilk on 2013-02-01.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include "bon.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>    // inf, nan etc
#include <assert.h>

// File size:
#include <sys/stat.h>
#include <sys/types.h>


#define FILE_NAME "foo.bon"


#define N_VECS 4

void writeStruct(bon_w_doc* doc)
{
	typedef struct {
		float    pos[3];
		float    normal[3];
		uint8_t  color[4];
	} Vertex;
	assert(sizeof(Vertex) == 3*sizeof(float) + 3*sizeof(float) + 4*sizeof(uint8_t));
	
	Vertex vecs[N_VECS];
	for (int i=0; i<N_VECS; ++i) {
		vecs[i].pos[0] = i;
		vecs[i].pos[1] = i;
		vecs[i].pos[2] = i;
		vecs[i].normal[0] = 100 + i;
		vecs[i].normal[1] = 100 + i;
		vecs[i].normal[2] = 100 + i;
		vecs[i].color[0] =  (uint8_t)i;
		vecs[i].color[1] =  (uint8_t)i;
		vecs[i].color[2] =  (uint8_t)i;
		vecs[i].color[3] =  (uint8_t)i;
	}
	
	bon_type*    type_vertex       =  bon_new_type_fmt("{[3f][3f][4u8]}", "pos", "normal", "color");
	bon_type*    type_vertex_list  =  bon_new_type_array(N_VECS, type_vertex);
	
	bon_w_aggregate(doc, type_vertex_list, vecs, sizeof(vecs));
	
	bon_free_type( type_vertex_list );
}

void readStruct(const bon_r_doc* doc, const bon_value* val)
{
	/*
	 Ideally, one would simply spec up the receiving struct and
	 ask BON to to read all matching elements, doing conversions if necessary.
	 */
	
	typedef struct {
		float    pos[3];
		uint8_t  color[4];
	} Vertex;
	assert(sizeof(Vertex) == 3*sizeof(float) + 4*sizeof(uint8_t));
	
	bon_type*    type_vertex       =  bon_new_type_fmt("{[3f][4u8]}", "pos", "color");
	bon_type*    type_vertex_list  =  bon_new_type_array(N_VECS, type_vertex);
	
	/* Before reading, we want to inquire about the size of what we are about to read to */
	bon_size size = bon_r_list_size(val);
	
	printf("%d vecs: \n", (int)size);
	
	Vertex* vecs = calloc(size, sizeof(Vertex));
	
#if 1
	if (bon_r_read_aggregate(doc, val, type_vertex_list, vecs, size * sizeof(Vertex))) {
		
		for (bon_size i=0; i<size; ++i) {
			printf("x: %.1f\n", vecs[i].pos[0]);
		}
	} else {
		fprintf(stderr, "Failed to read 'vecs'");
	}
#endif
}

void create()
{
#if 0
	bon_w_aggr_type_t*  type_vec3    =  bon_w_new_type_simple_array(3, BON_CTRL_FLOAT32);
	bon_w_aggr_type_t*  type_mat3x3  =  bon_w_new_type_array(3, type_vec3);
	
	float identity[3][3] = {{1,0,0}, {0,1,0}, {0,0,1}};
#endif
	
	FILE* fp = fopen(FILE_NAME, "wb");
	
	bon_w_doc* doc = bon_w_new_doc(&bon_file_writer, fp);
	bon_w_header(doc);
	bon_w_begin_obj(doc);
	
#if 1
	bon_w_key(doc, "null", BON_ZERO_ENDED);
	bon_w_null(doc);
	
	bon_w_key(doc, "false", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_FALSE);
	
	bon_w_key(doc, "true", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	
	bon_w_key(doc, "uint8", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7f);
	
	bon_w_key(doc, "sint8", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x80);
	
	bon_w_key(doc, "uint16", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fff);
	
	bon_w_key(doc, "sint16", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x8000);
	
	bon_w_key(doc, "uint32", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fffffff);
	
	bon_w_key(doc, "sint32", BON_ZERO_ENDED);
	bon_w_sint64(doc, -0x80000000LL);
	
	bon_w_key(doc, "uint64", BON_ZERO_ENDED);
	bon_w_uint64(doc, 0x7fffffffffffffffULL);
	
	bon_w_key(doc, "sint64", BON_ZERO_ENDED);
	bon_w_sint64(doc, (int64_t)-0x8000000000000000LL);
	
#ifdef NAN
	bon_w_key(doc, "nan", BON_ZERO_ENDED);
	bon_w_float(doc, NAN);
#endif
	
#ifdef INFINITY
	bon_w_key(doc, "pos_inf", BON_ZERO_ENDED);
	bon_w_float(doc, +INFINITY);
	
	bon_w_key(doc, "neg_inf", BON_ZERO_ENDED);
	bon_w_float(doc, -INFINITY);
#endif
	
	bon_w_key(doc, "list", BON_ZERO_ENDED);
	bon_w_begin_list(doc);
	bon_w_uint64(doc, 2);
	bon_w_uint64(doc, 3);
	bon_w_uint64(doc, 5);
	bon_w_uint64(doc, 7);
	bon_w_string(doc, "eleven", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "nested", BON_ZERO_ENDED);
	bon_w_uint64(doc, 13);
	bon_w_end_obj(doc);
	bon_w_end_list(doc);
	
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "and", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_key(doc, "and", BON_ZERO_ENDED);
	bon_w_bool(doc, BON_TRUE);
	bon_w_key(doc, "deeper", BON_ZERO_ENDED);
	bon_w_begin_obj(doc);
	bon_w_end_obj(doc);
	bon_w_end_obj(doc);
	bon_w_end_obj(doc);
#endif
	
#if 0
	bon_w_key(doc, "array", BON_ZERO_ENDED);
	double doubles[3] = {1.0f, 0.5f, 0.33333333333333333f};
	bon_w_array(doc, 3, BON_CTRL_FLOAT64, doubles, sizeof(doubles));
	
	bon_w_key(doc, "mat", BON_ZERO_ENDED);
	bon_w_aggregate(doc, type_mat3x3, identity, 3*3*sizeof(float));
#endif
	
	bon_w_key(doc, "vecs", BON_ZERO_ENDED);
	writeStruct(doc);
	
	bon_w_end_obj(doc);
	bon_w_close_doc(doc);
	
	fclose(fp);

#if 0
	bon_w_free_type(&type_mat3x3);
	bon_w_free_type(&type_vec3);
#endif
}

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

void read_bon()
{
	size_t size;
	const uint8_t* data = readFile(&size, FILE_NAME);
	bon_r_doc* doc = bon_r_open(data, size);
	
	const bon_value* root = bon_r_root(doc);
	const bon_value* vecs = bon_r_get_key(root, "vecs");
	readStruct(doc, vecs);
	
	bon_r_close(doc);
}

void bon_2_json()
{
	size_t size;
	const uint8_t* data = readFile(&size, FILE_NAME);
		
	bon_r_doc* doc = bon_r_open(data, size);
	
	bon_print(stdout, bon_r_root(doc), 0);
	
	bon_r_close(doc);
	
	//bon_r_close_doc(&doc);
}

void print_summary(const bon_value* v)
{
	// TODO: use bon_logical_type(val)
	
	switch (v->type) {
		case BON_VALUE_OBJ:
			printf("{ ... }");
			break;
			
		case BON_VALUE_LIST:
			printf("[ ... ]");
			break;
			
		case BON_VALUE_AGGREGATE:
			printf("aggr");
			break;
			
		default:
			bon_print(stdout, v, 0);
	}
}

void print_commands()
{
	printf(
			 "TODO: cd key    -  enter a subobject\n"
			 "exit            -  exit bon\n"
			 "help            -  prints this help\n"
			 "ls              -  list members of current object\n"
			 "stats           -  print statistics about the entire bon file\n"
			 "TODO: pwd       -  print path of current object\n"
			 "print key       -  full print of the value associated with the given key\n"
			 );
}

bon_bool open_file(const char* path) {
	size_t size;
	const uint8_t* data = readFile(&size, path);
	if (!data) {
		fprintf(stderr, "Failed to read .bon file at %s\n", path);
		return BON_FALSE;
	}
	
	bon_r_doc* doc   = bon_r_open(data, size);
	if (!doc) {
		fprintf(stderr, "Failed to parse .bon file at %s\n", path);
		return BON_FALSE;
	}
	
	const bon_value* root  = bon_r_root(doc);
	const bon_value* dir   = root;
	
	/* TODO: we should really use something like bon_r_as_obj(doc, root).
	 Why? So we can follow block refs and handle struct:s. */
	
	if (dir->type != BON_VALUE_OBJ) {
		fprintf(stderr, "Root value is not an object.\n");
		return BON_FALSE;
	}
	
	char line[256];
	
	while (fgets(line, sizeof(line), stdin)) {
		if (strcmp(line, "help\n")==0) {
			print_commands();
		} else if (strcmp(line, "exit\n")==0) {
			break;
		} else if (strcmp(line, "ls\n")==0) {
			if (dir->type == BON_VALUE_OBJ) {
				const bon_value_obj* o = &dir->u.obj;
				for (bon_size ki=0; ki<o->kvs.size; ++ki) {
					const bon_kv* kv = &o->kvs.data[ki];
					bon_print(stdout, &kv->key, 0);
					printf("              ");
					print_summary(&kv->val);
					printf("\n");
				}
				printf("\n");
			}
		} else if (strncmp(line, "print ", 6)==0) {
			const char* key_name = line+6;
			if (strcmp(key_name, ".\n")==0) {
				bon_print(stdout, dir, 0);
			} else {
				const bon_value* val = bon_r_get_key(dir, key_name);
				if (val) {
					bon_print(stdout, val, 0);
					printf("\n");
				} else {
					fprintf(stderr, "No such key (try \"ls\")\n");
				}
			}
		} else if (strncmp(line, "stats\n", 6)==0) {
			bon_stats* stats = &doc->stats;
			printf("-----------------\n");
			printf("%8d bytes in total\n",                  (int)stats->bytes_file);
			printf("%8d bytes in %d strings (incl header)\n",  (int)stats->bytes_string_wet, (int)stats->count_string);
			printf("%8d bytes in %d strings (excl header)\n",  (int)stats->bytes_string_dry, (int)stats->count_string);
			printf("%8d bytes in %d aggrs (incl header)\n",    (int)stats->bytes_aggr_wet, (int)stats->count_aggr);
			printf("%8d bytes in %d aggrs (excl header)\n",    (int)stats->bytes_aggr_dry, (int)stats->count_aggr);
			printf("-----------------\n");
		} else {
			printf("Unknown commands (try typing \"help\")\n");
		}
	}
	
	bon_r_close(doc);
	return BON_TRUE;
}

int main(int argc, const char * argv[])
{
	/*
	 usage:
	 
	 bon foo.bon
	 ls         - list keys and what they map to (unless too long)
	 cd         - open a child object
	 print key  - output a value/list/object as json 
	 exit
	 
	 or:
	 
	 bon -json -block0 foo.bon
	 
	 cat | json2bon | bar.fon
	 { "foo": 12 }^D
	 bon2json bar.fon
	
	 
	 // Print root block as json (i.e. skip trailing, big blocks)
	 
	 */
	
	
	create();
	//bon_2_json();
	read_bon();
	
	for (int i=1; i<argc; ++i) {
		if (argv[i][0] == '-') {
			printf("Usage: bon filename\n\n");
			print_commands();
		} else {
			if (!open_file(argv[i])) {
				return EXIT_FAILURE;
			}
		}
	}
	
	return 0;
}
