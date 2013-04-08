#include <bon.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	const char* path = (argc < 2 ? "foo.bon" : argv[1]);

	// Read file contents:
	size_t file_size;
	const uint8_t* file_data = bon_read_file(&file_size, path);

	if (!file_data) {
		fprintf(stderr, "Failed to read %s\n", path);
		return 1;
	}
	
	bon_r_doc* B = bon_r_open(file_data, file_size, BON_R_FLAG_DEFAULT);

	if (bon_r_error(B) != BON_SUCCESS) {
		fprintf(stderr, "Failed to parse BON file: %s\n", bon_err_str(bon_r_error(B)));
		bon_r_close(B);
		return 2;
	}

	// Get root object:
	bon_value* root = bon_r_root(B);
	
	// Retreive the key named 'msg':
	bon_value* msg = bon_r_get_key(B, root, "msg");

	if (bon_r_is_string(B, msg)) {
		// Print it out:
		printf("%s", bon_r_cstr(B, msg));
	}
	
	bon_r_close(B);
	free(file_data); // must be freed AFTER we close B!
	
	return 0;
}
