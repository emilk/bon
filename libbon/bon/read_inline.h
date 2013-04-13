//
//  read_inline.h
//  BON
//
//  Written 2013 by Emil Ernerfeldt.
//  Copyright (c) 2013 Emil Ernerfeldt <emil.ernerfeldt@gmail.com>
//  This is free software, under the MIT license (see LICENSE.txt for details).

#ifndef BON_read_inline_h
#define BON_read_inline_h



//------------------------------------------------------------------------------

BON_INLINE bon_bool bon_r_is_nil(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_NIL;
}

BON_INLINE bon_bool bon_r_is_bool(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_BOOL;
}

BON_INLINE bon_bool bon_r_is_int(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	return val->type == BON_VALUE_SINT64
	||     val->type == BON_VALUE_UINT64;
}

BON_INLINE bon_bool bon_r_is_number(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	return val->type == BON_VALUE_SINT64
	||     val->type == BON_VALUE_UINT64
	||     val->type == BON_VALUE_DOUBLE;
}

BON_INLINE bon_bool bon_r_is_double(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_DOUBLE;
}

BON_INLINE bon_bool bon_r_is_string(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	return val->type == BON_VALUE_STRING;
}

BON_INLINE bon_bool bon_r_is_list(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	if (val->type == BON_VALUE_LIST) { return BON_TRUE; }
	if (val->type == BON_VALUE_AGGREGATE) {
		const bon_value_agg* agg = val->u.agg;
		return agg->type.id == BON_TYPE_ARRAY;
	}
	return BON_FALSE;
}

BON_INLINE bon_bool bon_r_is_object(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_FALSE; }
	
	if (val->type == BON_VALUE_OBJ) { return BON_TRUE; }
	if (val->type == BON_VALUE_AGGREGATE) {
		const bon_value_agg* agg = val->u.agg;
		return agg->type.id == BON_TYPE_STRUCT;
	}
	return BON_FALSE;
}


BON_INLINE bon_logical_type  bon_r_value_type(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return BON_LOGICAL_ERROR; }
	
	switch (val->type)
	{
		case BON_VALUE_NIL:     { return BON_LOGICAL_NIL; }
		case BON_VALUE_BOOL:    { return BON_LOGICAL_BOOL; }
		case BON_VALUE_UINT64:  { return BON_LOGICAL_UINT; }
		case BON_VALUE_SINT64:  { return BON_LOGICAL_SINT; }
		case BON_VALUE_DOUBLE:  { return BON_LOGICAL_DOUBLE; }
		case BON_VALUE_STRING:  { return BON_LOGICAL_STRING; }
		case BON_VALUE_LIST:    { return BON_LOGICAL_LIST; }
		case BON_VALUE_OBJ:     { return BON_LOGICAL_OBJECT; }
			
		case BON_VALUE_AGGREGATE: {
			if (bon_r_is_list(B, val)) {
				return BON_LOGICAL_LIST;
			} else if (bon_r_is_object(B, val)) {
				return BON_LOGICAL_OBJECT;
			} else {
				return BON_LOGICAL_ERROR;
			}
		}
			
		default: {
			return BON_LOGICAL_ERROR;
		}
	}
}


//------------------------------------------------------------------------------


bon_value* bon_exploded_aggr(bon_r_doc* B, bon_value* val);

BON_INLINE bon_value* bon_r_follow_refs(bon_r_doc* B, bon_value* val)
{
	/* We should be protected from infinite recursion here,
	 since we check all blockRefId on reading,
	 ensuring they only point to blocks with higher ID:s. */
	while (val && val->type == BON_VALUE_BLOCK_REF && B->error==0)
	{
		val = bon_r_get_block(B, val->u.blockRefId);
	}
	return val;
}


BON_INLINE bon_value* follow_and_explode(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_AGGREGATE) {
		// If the user treats it like an object, convert it into one.
		val = bon_exploded_aggr(B, val);
	}
	
	return val;
}


//------------------------------------------------------------------------------

BON_INLINE bon_bool bon_r_bool(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_BOOL) {
		return val->u.boolean;
	} else {
		return BON_FALSE;
	}
}


BON_INLINE int64_t bon_r_int(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_UINT64) {
		return (int64_t)val->u.u64;
	} else if (val->type == BON_VALUE_SINT64) {
		return val->u.s64;
	} else if (val->type == BON_VALUE_DOUBLE) {
		return (int64_t)val->u.dbl;
	} else {
		return 0;
	}
}


BON_INLINE uint64_t bon_r_uint(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_UINT64) {
		return val->u.u64;
	} else {
		return (uint64_t)bon_r_int(B, val);
	}
}


BON_INLINE double bon_r_double(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_DOUBLE) {
		return val->u.dbl;
	} else if (val->type == BON_VALUE_UINT64) {
		return val->u.u64;
	} else if (val->type == BON_VALUE_SINT64) {
		return val->u.s64;
	} else {
		return 0;
	}
}


BON_INLINE float bon_r_float(bon_r_doc* B, bon_value* obj)
{
	return (float)bon_r_double(B, obj);
}



BON_INLINE bon_size bon_r_strlen(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return 0;
	} else if (val->type == BON_VALUE_STRING) {
		return val->u.str.size;
	} else {
		return 0;
	}
}


BON_INLINE const char* bon_r_cstr(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (!val) {
		return NULL;
	} else if (val->type == BON_VALUE_STRING) {
		return (const char*)val->u.str.ptr;
	} else {
		return NULL;
	}
}


BON_INLINE bon_size bon_r_list_size(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return 0; }
	
	switch (val->type) {
		case BON_VALUE_LIST:
			return val->u.list.size;
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = val->u.agg;
			if (agg->type.id == BON_TYPE_ARRAY) {
				return agg->type.u.array->size;
			} else {
				return 0;
			}
		}
			
		default:
			return 0;
	}
}


BON_INLINE bon_value* bon_r_list_elem(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	if (!val) { return NULL; }
	
	if (val->type == BON_VALUE_LIST)
	{
		const bon_list* vals = &val->u.list;
		if (ix < vals->size) {
			return &vals->data[ ix ];
		}
	}
	
	return NULL;
}


//------------------------------------------------------------------------------


// Number of keys-value pairs in an object
BON_INLINE bon_size bon_r_obj_size(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	if (!val) { return 0; }
	
	switch (val->type) {
		case BON_VALUE_OBJ:
			return val->u.obj.size;
			
		case BON_VALUE_AGGREGATE: {
			const bon_value_agg* agg = val->u.agg;
			if (agg->type.id == BON_TYPE_STRUCT) {
				return agg->type.u.strct->size;
			} else {
				return 0;
			}
		}
			
		default:
			return 0;
	}
}

// Return NULL if 'val' is not an object, or ix is out of range.
BON_INLINE const char* bon_r_obj_key(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	bon_obj* kvs = &val->u.obj;
	if (ix < kvs->size) {
		return kvs->data[ix].key;
	} else {
		return NULL;
	}
}

BON_INLINE bon_value* bon_r_obj_value(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	bon_obj* kvs = &val->u.obj;
	if (ix < kvs->size) {
		return &kvs->data[ix].val;
	} else {
		return NULL;
	}
}


BON_INLINE bon_value* bon_r_get_key(bon_r_doc* B, bon_value* val, const char* key)
{
	val = follow_and_explode(B, val);
	
	if (!val || val->type != BON_VALUE_OBJ) {
		return NULL;
	}
	
	const bon_obj* kvs  = &val->u.obj;
	
	for (bon_size i=0; i<kvs->size; ++i) {
		bon_kv* kv = &kvs->data[i];
		
		// The key should always be a string:
		if (strcmp(key, kv->key) == 0) {
			return &kv->val;
		}
	}
	
	return NULL;
}


//------------------------------------------------------------------------------


#endif
