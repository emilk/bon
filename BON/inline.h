//
//  inline.h
//  BON
//
//  Created by emilk on 2013-04-10.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#ifndef BON_inline_h
#define BON_inline_h

#include <string.h> // memcpy
#include "bon_private.h"

/*
 TODO: we have to inline a lot more things.
 I'm seeing winnings of 3x here for some things.
 */


//------------------------------------------------------------------------------

#include "write_inline.h"

//------------------------------------------------------------------------------

bon_value* bon_exploded_aggr(bon_r_doc* B, bon_value* val);

static inline bon_value* bon_r_follow_refs(bon_r_doc* B, bon_value* val)
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


static inline bon_value* follow_and_explode(bon_r_doc* B, bon_value* val)
{
	val = bon_r_follow_refs(B, val);
	
	if (val && val->type == BON_VALUE_AGGREGATE) {
		// If the user treats it like an object, convert it into one.
		val = bon_exploded_aggr(B, val);
	}
	
	return val;
}


static inline double bon_r_double(bon_r_doc* B, bon_value* val)
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

static inline float bon_r_float(bon_r_doc* B, bon_value* obj)
{
	return (float)bon_r_double(B, obj);
}

static inline bon_value* bon_r_list_elem(bon_r_doc* B, bon_value* val, bon_size ix)
{
	val = follow_and_explode(B, val);
	if (!val) { return NULL; }
	
	if (val->type == BON_VALUE_LIST)
	{
		const bon_value_list* vals = &val->u.list;
		if (ix < vals->size) {
			return &vals->data[ ix ];
		}
	}
	
	return NULL;
}

#endif
