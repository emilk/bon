//
//  util.c
//  BON
//
//  Created by emilk on 2013-02-01.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//
// Util app named 'bon' for inspecting bon files.

#include "bon.h"
#include "bon_private.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>    // inf, nan etc
#include <assert.h>

// File size:
#include <sys/stat.h>
#include <sys/types.h>



void print_summary(bon_value* v)
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
	bon_size size;
	const uint8_t* data = bon_read_file(&size, path);
	if (!data) {
		fprintf(stderr, "Failed to read .bon file at %s\n", path);
		return BON_FALSE;
	}
	
	bon_r_doc* B   = bon_r_open(data, size, BON_R_FLAG_DEFAULT);
	if (!B) {
		fprintf(stderr, "Failed to parse .bon file at %s\n", path);
		return BON_FALSE;
	}
	
	bon_value* root  = bon_r_root(B);
	bon_value* dir   = root;
	
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
					bon_kv* kv = &o->kvs.data[ki];
					printf("%s", kv->key);
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
				bon_value* val = bon_r_get_key(B, dir, key_name);
				if (val) {
					bon_print(stdout, val, 0);
					printf("\n");
				} else {
					fprintf(stderr, "No such key (try \"ls\")\n");
				}
			}
		} else if (strncmp(line, "stats\n", 6)==0) {
			bon_stats* stats = &B->stats;
			printf("-----------------\n");
			printf("%8d bytes in total\n",                  (int)stats->bytes_file);
			//printf("%8d bytes in %d strings (incl header)\n",  (int)stats->bytes_string_wet, (int)stats->count_string);
			printf("%8d bytes in %d strings (excl header)\n",  (int)stats->bytes_string_dry, (int)stats->count_string);
			//printf("%8d bytes in %d aggrs (incl header)\n",    (int)stats->bytes_aggr_wet, (int)stats->count_aggr);
			printf("%8d bytes in %d aggrs (excl header)\n",    (int)stats->bytes_aggr_dry, (int)stats->count_aggr);
			printf("-----------------\n");
		} else {
			printf("Unknown commands (try typing \"help\")\n");
		}
	}
	
	bon_r_close(B);
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
