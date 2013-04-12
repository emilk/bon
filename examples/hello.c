#include <bon/bon.h>
#include <stdio.h>

int main() {
	FILE* fp = fopen("hello.bon", "wb");
	bon_w_doc* B = bon_w_new_doc(&bon_file_writer, fp, BON_W_FLAG_DEFAULT );
	
	bon_w_begin_obj(B);  // The root object
	bon_w_key(B, "msg");
	bon_w_cstring(B, "Hello world!");
	bon_w_end_obj(B);
	
	bon_error err = bon_w_close_doc( B );
	if (err != BON_SUCCESS) {
		fprintf(stderr, "Failed to write to hello.bon: %s", bon_err_str(err));
	}
	fclose( fp );
}
