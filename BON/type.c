//
//  type.c
//  BON
//
//  Created by emilk on 2013-04-07.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include "bon.h"
#include "bon_private.h"
#include <inttypes.h>
#include <stdlib.h>       // malloc, free, calloc
#include <stdarg.h>       // va_list, va_start, va_arg, va_end
#include <string.h>       // strcmp



//------------------------------------------------------------------------------

void bon_free_type_insides(bon_type* t)
{
	switch (t->id) {
		case BON_TYPE_ARRAY:
			bon_free_type(t->u.array->type);
			free(t->u.array);
			break;
			
		case BON_TYPE_STRUCT: {
			for (bon_size ti=0; ti < t->u.strct->size; ++ti) {
				bon_free_type_insides( &t->u.strct->kts[ti].type );
			}
			free(t->u.strct);
		} break;
			
		default:
			break;
	}
}

void bon_free_type(bon_type* t) {
	bon_free_type_insides(t);
	free(t);
}

bon_type* bon_new_type_simple(bon_type_id id) {
	bon_type* t = calloc(1, sizeof(bon_type));
	t->id = id;
	return t;
}

bon_type* bon_new_type_simple_array(bon_size n, bon_type_id id) {
	return bon_new_type_array(n, bon_new_type_simple(id));
}

bon_type* bon_new_type_array(bon_size n, bon_type* type) {
	bon_type* t = calloc(1, sizeof(bon_type));
	t->id = BON_TYPE_ARRAY;
	t->u.array = calloc(1, sizeof(bon_type_array));
	t->u.array->size = n;
	t->u.array->type = type;
	return t;
}

#if 0
bon_type* bon_new_type_struct(bon_size n, const char** names, bon_type** types) {
	
	bon_type* t        =  calloc(1, sizeof(bon_type));
	t->id              =  BON_TYPE_STRUCT;
	t->u.strct         =  calloc(1, sizeof(bon_type_struct));
	t->u.strct->size   =  n;
	t->u.strct->keys   =  names;  // TODO: copy
	t->u.strct->types  =  types;  // TODO: copy
	return t;
}
#endif


bon_type* bon_new_type_fmt_ap_obj(const char** fmt, va_list* ap)
{
	if (**fmt != '{') {
		return NULL;
	}
	++*fmt;
		
	typedef struct {
		bon_size  size, cap;
		bon_kt*   data;
	} exp_strct;
	
	exp_strct expStrct = {0,0,0};
		
	for (;;) {
		switch (**fmt) {
			case '}': {
				++*fmt;
				
				bon_type_struct* strct = BON_ALLOC_TYPE(1, bon_type_struct);
				strct->size  =  expStrct.size;
				strct->kts   =  expStrct.data;
				
				bon_type* ret = BON_ALLOC_TYPE(1, bon_type);
				ret->id = BON_TYPE_STRUCT;
				ret->u.strct = strct;
				return ret;
			}
				
			case '\0':
				fprintf(stderr, "obj with no ending }\n");
				goto on_error;
				
			default: {				
				const char* key = va_arg(*ap, const char*);
				bon_type* tmp_type = bon_new_type_fmt_ap(fmt, ap);
				if (!tmp_type) {
					goto on_error;
				}
				
				BON_VECTOR_EXPAND(expStrct, bon_kt, 1);
				
				bon_kt* kt = expStrct.data + expStrct.size - 1;
				kt->key  = key;
				kt->type = *tmp_type;
				free(tmp_type);
			}
		}
	}
	
	
on_error:
	for (bon_size ix=0; ix<expStrct.size; ++ix) {
		bon_free_type_insides( &expStrct.data[ix].type );
	}
	return NULL;
}

bon_bool bon_parse_size(const char** fmt, bon_size* out)
{
	if (**fmt < '1' || '9' < **fmt) {
		return BON_FALSE;
	}
	
	bon_size size = (bon_size)(**fmt - '0');
	++*fmt;
	
	while ('0' <= **fmt && **fmt <= '9') {
		size *= 10;
		size += (bon_size)(**fmt - '0');
		++*fmt;
	}
	
	*out = size;
	return BON_TRUE;
}

bon_type* bon_new_type_fmt_ap(const char** fmt, va_list* ap)
{
	switch (**fmt) {
		case '{': {
			return bon_new_type_fmt_ap_obj(fmt, ap);
		}
			
		case '[': {
			++*fmt;
			
			bon_size n;
			if (**fmt == '#') {
				++*fmt;
				n = va_arg(*ap, bon_size);
			} else {
				if (!bon_parse_size(fmt, &n)) {
					return NULL;
				}
			}
			bon_type* t = bon_new_type_fmt_ap(fmt, ap);
			if (!t) {
				return NULL;
			}
			if (**fmt != ']') {
				fprintf(stderr, "malformed array\n");
				bon_free_type(t);
				return NULL;
			}
			++*fmt;
			return bon_new_type_array(n, t);
		}
			
		case 'b': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_BOOL);
		}
			
		case 'f': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_FLOAT32);
		}
			
		case 'd': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_FLOAT64);
		}
			
		case 's': {
			++*fmt;
			return bon_new_type_simple(BON_TYPE_STRING);
		}
			
		case 'u':
		case 'i':
		{
			bon_bool sgnd = (**fmt == 'i');
			++*fmt;
			
			bon_size n;
			if (!bon_parse_size(fmt, &n)) {
				return NULL;
			}
			switch (n) {
				case 8:   return bon_new_type_simple(sgnd ? BON_TYPE_SINT8  : BON_TYPE_UINT8 );
				case 16:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT16 : BON_TYPE_UINT16);
				case 32:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT32 : BON_TYPE_UINT32);
				case 64:  return bon_new_type_simple(sgnd ? BON_TYPE_SINT64 : BON_TYPE_UINT64);
				default:
					fprintf(stderr, "Bad integer size: %d\n", (int)n);
					return NULL;
			}
		}
			
		default:
			fprintf(stderr, "Unknown format\n");
			return NULL;
	}
}

bon_type* bon_new_type_fmt(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bon_type* type = bon_new_type_fmt_ap(&fmt, &ap);
	va_end(ap);
	return type;
}

/* the size of the payload. */
bon_size bon_aggregate_payload_size(const bon_type* type)
{
	switch (type->id) {
		case BON_TYPE_ARRAY:
			return type->u.array->size *
			bon_aggregate_payload_size(type->u.array->type);
			
		case BON_TYPE_STRUCT: {
			bon_size sum = 0;
			bon_type_struct* strct = type->u.strct;
			for (bon_size i=0; i<strct->size; ++i) {
				sum += bon_aggregate_payload_size( &strct->kts[i].type );
			}
			return sum;
		}
			
		default:
			return bon_type_size( type->id );
	}
}

bon_bool bon_type_eq(const bon_type* a, const bon_type* b)
{
	if (a==b)            { return BON_TRUE;  }
	if (!a || !b)        { return BON_FALSE; }
	if (a->id != b->id)  { return BON_FALSE; }
	
	if (a->id == BON_TYPE_ARRAY) {
		if (a->u.array->size != b->u.array->size)  { return BON_FALSE; }
		return bon_type_eq(a->u.array->type, b->u.array->type);
	} else if (a->id == BON_TYPE_STRUCT) {
		const bon_type_struct* as = a->u.strct;
		const bon_type_struct* bs = b->u.strct;
		if (as->size != bs->size)  { return BON_FALSE; }
		for (bon_size ix=0; ix<as->size; ++ix) {
			if (strcmp(as->kts[ix].key, bs->kts[ix].key) != 0)       { return BON_FALSE; }
			if (!bon_type_eq(&as->kts[ix].type, &bs->kts[ix].type))  { return BON_FALSE; }
		}
		return BON_TRUE;
	} else {
		return a->id == b->id;
	}
}

//------------------------------------------------------------------------------

