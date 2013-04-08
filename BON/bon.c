//
//  bon.c
//  BON
//
//  Created by emilk on 2013-02-01.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//


#include "bon.h"
#include "bon_private.h"
#include <math.h>      // isnan, isinf


// For printf:ing int64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


//------------------------------------------------------------------------------

void bon_onError(const char* msg)
{
#ifdef DEBUG
	fprintf(stderr, "BON error: %s\n", msg);
	
	//__asm__("int $3\n" : : );  // breakpoint
#endif
}



//------------------------------------------------------------------------------


const char* bon_err_str(bon_error err)
{
	if (BON_NUM_ERR <= err)
		return "BON_UNKNOWN_ERROR";
	
	const char* err_str[BON_NUM_ERR] = {
		"BON_SUCCESS",
		
		"BON_ERR_WRITE_ERROR",
		"BON_ERR_BAD_AGGREGATE_SIZE",
		
		"BON_ERR_BAD_CRC",
		"BON_ERR_TOO_SHORT",
		"BON_ERR_BAD_HEADER",
		"BON_ERR_BAD_FOOTER",
		"BON_ERR_BAD_VLQ",
		"BON_ERR_MISSING_LIST_END",
		"BON_ERR_MISSING_OBJ_END",
		"BON_ERR_BAD_CTRL",
		"BON_ERR_BAD_KEY",
		"BON_ERR_BAD_AGGREGATE_TYPE",
		"BON_ERR_BAD_TYPE",
		"BON_ERR_STRING_NOT_ZERO_ENDED",
		"BON_ERR_MISSING_TOKEN",
		
		"BON_ERR_TRAILING_DATA",
		"BON_ERR_BAD_BLOCK",
		"BON_ERR_BAD_BLOCK_REF",
		
		"BON_ERR_NARROWING",
		"BON_ERR_NULL_OBJ",
	};
	
	return err_str[err];
}


//------------------------------------------------------------------------------
// Debug printing


void bon_print_float(FILE* out, double dbl)
{
	if (isnan(dbl))      fprintf(out, "NaN");
	else if (isinf(dbl)) fprintf(out, dbl < 0 ? "-Inf" : "+Inf");
	else                 fprintf(out, "%f", dbl);
}

void bon_print_aggr(FILE* out, const bon_type* aggType, bon_reader* br)
{
	uint8_t id = aggType->id;
	
	switch (id) {
		case BON_TYPE_ARRAY: {
			fprintf(out, "[");
			bon_type_array* arr = aggType->u.array;
			for (uint64_t ti=0; ti<arr->size; ++ti) {
				bon_print_aggr(out, arr->type, br);
				if (ti != arr->size-1)
					fprintf(out, ", ");
			}
			fprintf(out, "]");
		} break;
			
			
		case BON_TYPE_STRUCT: {
			fprintf(out, "{");
			bon_type_struct* strct = aggType->u.strct;
			for (uint64_t ti=0; ti<strct->size; ++ti) {
				bon_kt* kt = &strct->kts[ti];
				fprintf(out, "\"%s\": ", kt->key);
				bon_print_aggr(out, &kt->type, br);
				if (ti != strct->size-1)
					fprintf(out, ", ");
			}
			fprintf(out, "}");
		} break;
			
		case BON_CTRL_SINT8:
		case BON_CTRL_SINT16_LE:
		case BON_CTRL_SINT16_BE:
		case BON_CTRL_SINT32_LE:
		case BON_CTRL_SINT32_BE:
		case BON_CTRL_SINT64_LE:
		case BON_CTRL_SINT64_BE: {
			int64_t s64 = br_read_sint64(br, id);
			fprintf(out, "%"PRIi64, s64);
		}
			
			
		case BON_CTRL_UINT8:
		case BON_CTRL_UINT16_LE:
		case BON_CTRL_UINT16_BE:
		case BON_CTRL_UINT32_LE:
		case BON_CTRL_UINT32_BE:
		case BON_CTRL_UINT64_LE:
		case BON_CTRL_UINT64_BE: {
			uint64_t u64 = br_read_uint64(br, id);
			fprintf(out, "%"PRIu64, u64);
		} break;
			
			
		case BON_CTRL_FLOAT32_LE:
		case BON_CTRL_FLOAT32_BE:
		case BON_CTRL_FLOAT64_LE:
		case BON_CTRL_FLOAT64_BE: {
			double dbl = br_read_double(br, id);
			bon_print_float(out, dbl);
		} break;
			
			
		default: {
			fprintf(out, "2JSON_ERROR");
		}
	}
}

// Will print in json format.
void bon_print(FILE* out, bon_value* v, size_t indent)
{
#define INDENT "\t"
#define PRINT_NEWLINE_INDENT fprintf(out, "\n"); for (size_t iix=0; iix<indent; ++iix) fprintf(out, INDENT);
	
	switch (v->type) {
		case BON_VALUE_NIL:
			fprintf(out, "nil");
			break;
			
			
		case BON_VALUE_BOOL:
			fprintf(out, v->u.boolean ? "true" : "false");
			break;
			
			
		case BON_VALUE_UINT64:
			fprintf(out, "%"PRIu64, v->u.u64);
			break;
			
			
		case BON_VALUE_SINT64:
			fprintf(out, "%"PRIi64, v->u.s64);
			break;
			
			
		case BON_VALUE_DOUBLE:
			bon_print_float(out, v->u.dbl);
			break;
			
			
		case BON_VALUE_STRING:
			fprintf(out, "\"");
			fwrite(v->u.str.ptr, v->u.str.size, 1, out);
			fprintf(out, "\"");
			break;
			
			
		case BON_VALUE_LIST: {
			fprintf(out, "[ ");
			const bon_value_list* vals = &v->u.list;
			bon_size size = vals->size;
			for (bon_size i=0; i<size; ++i) {
				bon_print(out, vals->data + i, indent);
				if (i != size-1)
					fprintf(out, ", ");
			}
			fprintf(out, " ]");
		} break;
			
			
		case BON_VALUE_AGGREGATE: {
			bon_size byteSize = bon_aggregate_payload_size(&v->u.agg.type);
			bon_reader br = { v->u.agg.data, byteSize, 0, BON_BAD_BLOCK_ID, 0 };
			bon_print_aggr(out, &v->u.agg.type, &br);
			if (br.nbytes!=0) {
				fprintf(stderr, "Bad aggregate\n");
			}
		} break;
			
			
		case BON_VALUE_OBJ: {			
			const bon_kvs* kvs = &v->u.obj.kvs;
			bon_size size = kvs->size;
			
			if (size==0) {
				fprintf(out, "{ }");
			} else {
				bon_bool multiline = (size > 1);
				
				if (multiline) {
					fprintf(out, "{");
					indent++;
					PRINT_NEWLINE_INDENT
				} else {
					fprintf(out, "{ ");
				}
				
				for (bon_size i=0; i<size; ++i) {
					bon_kv* kv = &kvs->data[i];
					
					fprintf(out, "\"");
					fprintf(out, "\"%s\": ", kv->key);
					fprintf(out, "\": ");
					bon_print(out, &kv->val, indent);
					
					if (i != size-1) {
						fprintf(out, ",");
						
						if (multiline) {
							PRINT_NEWLINE_INDENT
						}
					}
				}
				
				if (multiline) {
					indent--;
					PRINT_NEWLINE_INDENT
					fprintf(out, "}");
				} else {
					fprintf(out, " }");
				}
			}
		} break;
			
		case BON_VALUE_BLOCK_REF: {
			fprintf(out, "@%"PRIu64, v->u.blockRefId);
		}
			
		default:
			fprintf(out, "ERROR");
	}
}
