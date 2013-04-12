//
//  util.c
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).
//
//  Util app named 'bon' for inspecting bon files.

#include <bon/bon.h>
#include <bon/private.h>  // bon_stats
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>    // inf, nan etc
#include <assert.h>

typedef struct elem_t elem_t;

#define MAX_PATH_COMP_LEN 32

struct elem_t
{
	bon_value*  val;
	elem_t*     prev;
	elem_t*     next;
	
	char descr[MAX_PATH_COMP_LEN+1]; // "how did we get here?"
};


// Globals:
bon_r_doc* B             = NULL;
elem_t*    g_stack_root  = NULL;
elem_t*    g_stack_top   = NULL;

bon_value* top() {
	return g_stack_top->val;
}

void pwd()
{
	elem_t* e = g_stack_root;
	while (e) {
		printf("%s/", e->descr);
		e = e->next;
	}
	printf("\n");
}


bon_bool open_file(const char* path) {
	bon_size size;
	const uint8_t* data = bon_read_file(&size, path);
	if (!data) {
		fprintf(stderr, "Failed to read .bon file at %s\n", path);
		return BON_FALSE;
	}
	
	B = bon_r_open(data, size, BON_R_FLAG_DEFAULT);
	bon_error err = bon_r_error(B);
	if (err != BON_SUCCESS) {
		fprintf(stderr, "Failed to parse .bon file at %s: %s\n", path, bon_err_str(err));
		bon_r_close(B);
		return BON_FALSE;
	}
	
	g_stack_root = (elem_t*)calloc(1, sizeof(elem_t));
	g_stack_root->val = bon_r_root(B);
	//strcpy(g_stack_root->descr, "/");
	g_stack_top = g_stack_root;
	return BON_TRUE;
}


void print_commands()
{
	printf(
			 "cd key          -  enter an element in an object. 'key' can also be a list index.\n"
			 "exit            -  exit bon\n"
			 "help            -  prints this help\n"
			 "ls              -  list members of current object\n"
			 "stats           -  print statistics about the entire bon file\n"
			 "pwd             -  print path of current object\n"
			 "print key       -  full print of the value associated with the given key. 'key' can also be a list index.\\n"
			 );
}


void print_summary(bon_value* v)
{
	switch (bon_r_value_type(B, v))
	{			
		case BON_LOGICAL_LIST:
			printf("[ ... ]  (list with %d elements)", (int)bon_r_list_size(B, v));
			break;
			
		case BON_LOGICAL_OBJECT:
			printf("{ ... }  (object with %d keys)", (int)bon_r_obj_size(B, v));
			break;
			
		default:
			bon_print(B, v, stdout, 0);
			break;
	}
}


void ls_obj(bon_value* obj)
{
	size_t size = bon_r_obj_size(B, obj);
	
	const size_t KeyValDist = 32;
	printf("KEYS");
	for (size_t si=0; si<KeyValDist; ++si) {
		printf(" ");
	}
	printf("VALUES\n");
	
	for (size_t ki=0; ki<size; ++ki) {
		const char* key = bon_r_obj_key  (B, obj, ki);
		bon_value*  val = bon_r_obj_value(B, obj, ki);
		
		size_t key_len = strlen(key);
		printf("  \"%s\": ", key);
		
		size_t spacing = KeyValDist - key_len;
		for (size_t si=0; si<spacing; ++si) {
			printf(" ");
		}
		
		print_summary(val);
		printf("\n");		
	}
}


void ls_list(bon_value* list)
{
	size_t size = bon_r_list_size(B, list);
	
	for (size_t ix=0; ix<size; ++ix) {
		bon_value*  val = bon_r_list_elem(B, list, ix);
		printf("%4d:  ", (int)ix);
		print_summary(val);
		printf("\n");
	}
}


void ls(bon_value* obj)
{
	if (bon_r_is_object(B, obj)) {
		ls_obj(obj);
	} else if (bon_r_is_list(B, obj)) {
		ls_list(obj);
	} else {
		fprintf(stderr, "You can only ls an object or a list");
	}
}

bon_bool all_int(const char* str)
{
	while (*str) {
		if (*str < '0'  || '9' < *str) {
			return BON_FALSE;
		}
		++str;
	}
	return BON_TRUE;
}

bon_value* val_by_name_or_num(const char* name)
{
	bon_value* dir = top(); // current dir
	
	if (strcmp(name, ".")==0) { return dir; }
	if (strcmp(name, "..")==0) { return g_stack_top->val; }
	
	bon_value* val = bon_r_get_key(B, dir, name);
	if (val) { return val; }
	
	// Try using as a number:
	if (all_int(name)) {
		bon_size ix = (bon_size)atoi(name);
		
		if (bon_r_is_object(B, dir)) {
			return bon_r_obj_value(B, dir, ix);
		} else {
			return bon_r_list_elem(B, dir, ix);
		}
	}
	
	return NULL;
}

void cd(const char* key)
{
	if (strcmp(key, ".")==0) {
		return;
	}
	
	if (strcmp(key, "..")==0) {
		elem_t* oldTop = g_stack_top;
		g_stack_top = oldTop->prev;
		free(oldTop);
		g_stack_top->next = NULL;
		return;
	}
	
	bon_value* val = val_by_name_or_num(key);
	
	if (val) {
		//bon_value* old = top();
		
		g_stack_top->next = (elem_t*)calloc(1, sizeof(elem_t));
		g_stack_top->next->val = val;
		g_stack_top->next->prev = g_stack_top;
		g_stack_top = g_stack_top->next;
		
		size_t len = strlen(key);
		if (len > MAX_PATH_COMP_LEN) {
			len = MAX_PATH_COMP_LEN;
		}
		strncpy(g_stack_top->descr, key, len);
		g_stack_top->descr[len] = 0;
	} else {
		fprintf(stderr, "No such key or index \"%s\" (try ls)\n", key);
	}
}

void stats()
{
	bon_stats* stats = &B->stats;
	printf("-----------------\n");
	printf("%8d bytes in total\n",                  (int)stats->bytes_file);
	//printf("%8d bytes in %d strings (incl header)\n",  (int)stats->bytes_string_wet, (int)stats->count_string);
	printf("%8d bytes in %d strings (excl header)\n",  (int)stats->bytes_string_dry, (int)stats->count_string);
	//printf("%8d bytes in %d aggrs (incl header)\n",    (int)stats->bytes_aggr_wet, (int)stats->count_aggr);
	printf("%8d bytes in %d packed format (excl header)\n",    (int)stats->bytes_aggr_dry, (int)stats->count_aggr);
	printf("-----------------\n");
}


void run()
{
	char line[256];
	
	//printf("> ");
	
	while (fgets(line, sizeof(line), stdin)) {
		bon_value* dir = top(); // current dir
		
		// Remove trailing endline:
		line[strlen(line) - 1] = 0;
		
		if (strlen(line)==0) { continue; }  // Enter
		
		if (strcmp(line, "help")==0) {
			print_commands();
		} else if (strcmp(line, "exit")==0) {
			break;
		} else if (strcmp(line, "ls")==0) {
			ls(dir);
		} else if (strcmp(line, "pwd")==0) {
			pwd();
		} else if (strncmp(line, "print ", 6)==0) {
			const char* key = line+6;
			bon_value* val = val_by_name_or_num(key);
			
			if (val) {
				bon_print(B, val, stdout, 0);
				printf("\n");
			} else {
				fprintf(stderr, "No such key or index \"%s\" (try ls)\n", key);
			}
		} else if (strncmp(line, "cd ", 3)==0) {
			const char* key = line+3;
			cd(key);
		} else if (strncmp(line, "stats", 6)==0) {
			stats();
		} else {
			printf("Unknown commands (try typing \"help\")\n");
		}
		
		//printf("> ");
	}
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
	 
	 cat | json2bon > bar.bon
	 { "foo": 12 }^D
	 bon bar.bon
	 
	 
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
			
			run();
			
			bon_r_close(B);
		}
	}
	
	return 0;
}
