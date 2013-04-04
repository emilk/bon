BON - Binary Object Notation
============================

A general purpose binary, serialization format
-----------------------------

# Caution - EARLY DRAFT
This is an early draft ,and should be read as such. Many things are missing or left unspecified.

# What is this?
BON is a binary format for serializing data for storage or communication. It is in many ways a binary analog to JSON, and attempts to remedy the shortcomings of other formats, such as BSON and MessagePack, while still being simple, unlike more advanced formats such as HDF5. It is also designed to be dynamic, as messages can be composed on the fly. This is in contrast with e.g. Google's Protocol Buffers, where you statically generate code for encoding/decoding a predefined set of messages.


## MessagePack
BON is highly influenced by MessagePack, and it might be worth to read the [MessagePack format specification](http://wiki.msgpack.org/display/MSGPACK/Format+specification) before reading the BON specification. BON attempts to remedy the following limitations of MessagePack:

* No self-identifying header
	* A four byte magic header identifies BON - no such luck for MessagePack.
* Bad unicode support
	* In MessagePack there is not string type, only a byte data type, for which one cannot assume an encoding (UTF8, UTF16 etc)
	* Further discussion [here](https://github.com/msgpack/msgpack/issues/121#).
* Slow on big endian machines
	* One has to convert to little endian in MessagePack - BON handles big and little endian natively.
* No support for large data blocks
	* In MessagePack you cannot write a million floating points numbers in an efficient way - one will have to insert a one-byte float header for each number.
	* In BON one can write the same data as as homogenous array with zero overhead.
* No support for streaming
	* One cannot stream a MessagePack list since any such list much has the size set first. BON supports full streaming of lists and objects/maps.
	
	
## Goals
* Binary
	* Binary is faster, more compact, and more secure (see appendix)
* Simple
	* Not as simple as MessagePack, but simpler than HDF5
* Fast
	* Write and read quickly
	* Zero-copy for reading any value (including strings, which has mandatory zero byte ending).
* Secure
	* No more security holes due to problematic string escaping (see appendix)
* Flexibility, chose to:
	* Optimize for small output
	* Optimize for output that can be changed in-place (i.e. not use FixInts)
	* Optimize for streaming serialization
	* Optimize for skipping parsing (putting big data blocks after structural data blocks)
	* Optimize for huge files to which one can append more data
* 64 bit support, both in integers and list sizes
* Supports both big and little endian, for interoperability and speed
* Naturally unicode (all strings are UTF8)
* Full bidirectional compatibility with JSON
* Compact
	* Minimal overhead
	* More compact than BSON for JSON-like data
	* Allow encoder to optimize for size when encoding homogenous data (e.g. arrays of integers)
* Self-identifying (four byte magic header)
* Self-describing
	* Like JSON, a BON message can be self-explanatory by having good key names.
* Heterogeneous arrays
	* Allows large blobs of data to be copied right into a native holder
	* Less overhead than MessagePack
* Streaming serialization and deserialization
* Frequent sanity checks
* Dynamic
	* Messages can be created on the fly


## Non-goals
* More than JSON (so still no date type)
* Compression
	* Makes the format and its use overly-complicated
	* BON prioritizes speed and simplicity
* Recoverability after corruption
	* Such a format would be unsecure anyway (how would one know one is not parsing user data after recovering from previous errors?)
	
	
## Use examples
* Messages (e.g. server-client communication)
* Storage of user records
* Storage of uncompressed data, like:
	* Sound, 3D models, images, vector graphics
	* Scientific data, e.g. dense and sparse matrices	
	
	
# Format	
## Values / Types:
### Atomic:
* Nil
* True
* False
* Integers, all combinations of:
	* signed/unsigned
	* 8,16,32,64 bit
	* Big / Little endian
* Float, all combinations of:
	* 32 or 64 bit  (single precision, double precision)
	* Big / Little endian


### Flexible:
* String
	* Prefixed with size
	* UTF8-encoded
	* Zero ended
* Object (aka “map”)
	* Maps string keys to values of any type
	* Ended by an END_OBJECT control byte
* List (heterogenous)
	* Any number of values of any type (mixing of types allowed)
	* Ended by an END_LIST control byte


### Types for optimizing storage of homogenous lists.
These are optimized for storing chunks of binary data. They store the elements without individual type-prefixes.

* Array (homogenous)
	* Prefixed with size (number of elements)
	* Element type (integers, floats, arrays and structs only)
* Struct - for interleaved data in Arrays
	* Prefixed with the number of fields
	* A series of name-type pairs (integers, floats, arrays and structs)


Note: sizes and nested types are part of the parent type.


## Example BON file (no blocks):

	BON0
	OBJ_BEGIN
		STRING_4 “name” 0        STRING_3 “BON” 0x00
		STRING_7 “version” 0     FIXINT_1
		STRING_9 “two bytes” 0   BYTE_ARRAY_2
			0x12 0x13
		STRING_4 “ages” 0
			OBJ_BEGIN
        	    STRING_4 “Emil” 0   FIXINT_27
        	    STRING_4 “John” 0   FIXINT_29
        	OBJ_END
		STRING_17 “identity matrices” 0
			LIST_BEGIN
				ARRAY_1 ARRAY_1 FLOAT32 1
				ARRAY_2 ARRAY_2 FLOAT32 1 0 1 0
				ARRAY_3 ARRAY_3 FLOAT32 1 0 0 1 0 0 1 0 0
			LIST_END
	OBJ_END
	FOOTER

This is equivalent to the JSON:

	{
		"name" : "BON",
		"version" : 1,
		"two bytes" : [18, 19],
		"ages" : {
			"Emil" : 27,
			"John" : 29
		},
		"identity matrices" : [
			[1],
			[[1,0], [0,1]],
			[[1,0,0], [0,1,0], [0,0,1]]
		]
	}


Words in ALL_CAPS are single byte control codes. Strings are prefixed by a control byte which include their size, and are followed by a zero byte.

Things to note: the 3x3 float matrix need only a 3 byte header, then all nine floats follows in memory, meaning one can copy it directly from/to a float[3][3] in e.g. C.

Examples from now on will ommit the leading string size and following zero byte.


## Example of using arrays and structs
Let say you want to store the vertices of a 3D model in a BON file. Each vertex has a position and a color like such:

	struct Vertex {
		float pos[3];
		uint8_t color[4];
	};
	Vertex vecs[1024];

The straightforward way to encode this is to store each Vertex as a separate object, like such:

	“vecs” : LIST_BEGIN
		OBJ_BEGIN
			“pos” : LIST_BEGIN
				FLOAT32_LE [x] FLOAT32_LE [y] FLOAT32_LE [z]
			LIST_END
			“color” : LIST_BEGIN
				UINT8 [r] UINT8 [g] UINT8 [b] UINT8 [a]
			LIST_END
		OBJ_END
		OBJ_BEGIN
			“pos” : LIST_BEGIN ...

...and so on. Obviously we are doing a lot of work encoding this, as well as making the BON file unnecessarily large. Another way would be to encode the entire ‘vecs’ structure as raw bytes and let the user figure it all out:

	“vecs” : ARRAY_VLQ [sizeof(vecs)] UINT8 [raw byte data]

But now we loose all that information about what is encoded. What if a later version changes ‘pos’ to be encoded as double instead of float? What if another field is added to the Vertex struct?

This is where Array:s of Struct:s come to the rescue:

	“vecs” : ARRAY_VLQ 1024 STRUCT_VLQ 2
		“pos” ARRAY_3 FLOAT32_LE
		“color” BYTE_ARRAY_4
		[raw byte data]

This is logically equivalent to the first version, but stored much much more efficiently. The user can write the entire structure in one fell swoop. The reader can check if the structure is as expected before reading and, if it is, just copy the data right into place.

Note that the struct declaration has the same basic layout as an object, but with the actual values postponed.

Arrays of structs is a very efficient way to encode homogenous elements of data (data in which each element are of the same type). Note that the array and struct elements must be of a fixed size, so they cannot contain strings, lists or objects - nor FixInt:s or true/false/nil.


## Blocks (optional feature - not quite finished)
When reading a huge BON file, it may be inconvenient to have to parse the whole file just to access a part of it. To help with this issue, a BON file can optionally be structured into blocks.

Each block is encoded with a unique id (an unsigned integer, up to 64 bits) and a size in bytes.

A reference to a block is considered a value. So the root object can for instance map some keys to blocks, and the parser can selectively parse only those block requested by the user.

Note that block declarations cannot be nested - they are top level structures.

Here’s an example of a blocked BON file for storing user names and images:

	BON0

	BLOCK_BEGIN 0 25			// id #0 (root) and byte size of block
		OBJ_BEGIN
		“users”
			OBJ_BEGIN
				“john” @1		// block reference
				“paul” @2		// ditto
			OBJ_END
		OBJ_END
	BLOCK_END

	BLOCK_BEGIN 1 1000048		// id #1 byte size of block
		OBJ_BEGIN
			“first” “John”
			“second” “Lennon”
			“image.jpg” ARRAY_VLQ 1000000 UINT8 [data]
		OBJ_END
	BLOCK_END
	
	BLOCK_BEGIN 2 2000051		// id #1 byte size of block
		OBJ_BEGIN
			“first” “Paul”
			“second” “McCartney”
			“image.jpg” ARRAY_VLQ 2000000 UINT8 [data]
		OBJ_END
	BLOCK_END



When the user now wants to access an image of Paul, the parser can completely skip over parsing the block for John.

Blocks can refer to other blocks, but the references may not form cycles (so the structure is a DAG). Even when several block refers to the same block, the logical structure is still that of a tree. For instance, when transforming BON to JSON, a block referenced from several other blocks will be represented multiple times in the JSON document. Therefore, blocks can also be used as a form of compression.

Block sizes are excluding the BLOCK_BEGIN and BLOCK_END, and also excluding the VLQ:s encoding the id and block size. The size, in other words, is of the payload - the contained value. A block-size of 0 is reserved to mean "undefined size - parse to BLOCK_END".

BLOCK_END is only there as a sanity check.

Block ID:s can be any unsigned integer (less than 2^64), but 0 is reserved for the root block.


## More about blocks
One nice feature is to be able to append data to a .BON file without rewriting it from scratch. The way to implement that is to have the root block last in the BON file.

	HEADER
	BLOCK #1
	BLOCK #2
	...
	BLOCK #0

To append a block of data:

1. read and remove the root block (BLOCK #0)
2. append the new data block
3. ammend the root object with a reference to the new data block
4. append BLOCK #0



# JSON interoperability
Roundtripping correctly with JSON is important for inspecing and editing smaller BON documents. Here's the limitations:

### JSON -> BON -> JSON
* Numbers may be encoded differently, but with the same value (i.e. 0.000 may become 0.0)
* All forward slashes will be escaped in output

### BON -> JSON -> BON
* Problem: NaN/Inf floats are supported by BON, but not by JSON
	* Damn JSON for not supporting these. Discussion [here](http://lavag.org/topic/16217-cr-json-labview/?p=99058).
	* BON->JSON, encode:
		* +Inf as  1e99999  (will overflow naturally to infinity)
		* -Inf as -1e99999  (will underflow naturally to negative infinity)
		* NaN  as  0e666    (will be decoded as zero by standard JSON parser)
	* JSON->BON:
		* Helpfully detect the above notations


# Codes

The positions of the FixInt:s (negative and positive) have been chosen so that a control byte encoding a FixInt can be referred to directly as an sint8.

The positions of the control blocks have been chosen to coincide with the ASCII letters, to make control codes easily findable in debug output. Also, this allows our header (which should be ASCII to work as magic file id) to be a normal control code like any other, making it orthogonal.

We also optimize for small arrays, structs, strings and block refs. Block refs in particular is nice for to enable aggressive compression of common keys and values. For instance, if you have lot of object on the form
`{ “state”: “open”, “id” : 0 }, { “state”: “closed”, “id” : 1}, { “state”: “unknown”, “id” : 2}`
then naively these will take up about 20-25 bytes each. But if the encoder assign blocks 1-5 to the above strings (`"state", "open", "id", "closed", "unknown"`), we can make each object just six bytes:
 `OBJ_BEGIN REF_1 REF_2 REF_3 num OBJ_END` 

### Control byte ranges and meanings:

| Mask		| Type			| Range (inclusive)
| --------	| ------------ 	| ----- 
| 000xxxxx	| PosFixInt	 	| [0, 31]
| 001xxxxx	| FixString		| [0, 31]
| 01xxxxxx	| Control codes	| 64 codes (some reserved)
| 10xxxxxx	| FixBlockRef	| [0, 63]
| 1100xxxx	| FixArray		| [0, 15]
| 1101xxxx	| FixByteArray	| [0, 15]
| 1110xxxx	| FixStruct		| [0, 15]
| 1111xxxx	| NegFixInt 	| [-16,-1]

### Control codes (expressed in ASCII):

	BLOCK_REF   = '@',    STRING_VLQ  = '\'',  // 0x40  0x60
	ARRAY_VLQ   = 'A',
	HEADER      = 'B',
	STRUCT_VLQ  = 'C',
	BLOCK_BEGIN = 'D',    BLOCK_END   = 'd',
	TRUE        = 'E',    FALSE       = 'e',
	NIL         = 'N',
	
	SINT8       = 'P',
	UINT8       = 'Q',
	SINT16_LE   = 'R',    SINT16_BE   = 'r',
	UINT16_LE   = 'S',    UINT16_BE   = 's',
	SINT32_LE   = 'T',    SINT32_BE   = 't',
	UINT32_LE   = 'U',    UINT32_BE   = 'u',
	SINT64_LE   = 'V',    SINT64_BE   = 'v',
	UINT64_LE   = 'W',    UINT64_BE   = 'w',
	
	FLOAT32_LE  = 'X',    FLOAT32_BE  = 'x',
	FLOAT64_LE  = 'Y',    FLOAT64_BE  = 'y',
	
	LIST_BEGIN  = '[',    LIST_END    = ']',  // 0x5B   0x5D
	OBJ_BEGIN   = '{',    OBJ_END     = '}',  // 0x7B   0x7D

All other values in [64, 127] are reserved for future use. Any decoder encountering such a value should abort.



# Appendix: Design decisions

* Strings have both length and are zero ended
	* Just zero-ended is not good enough:
		* 0 is a valid unicode codepoint
		* A potential attack is to add zeros to unicode strings before encoding with BON.
	* Having an extra zero makes reading strings a lot simpler in many languages where zero-ended strings are the default (e.g. C), and thus also adds an extra layer of security.
* Strings are always UTF-8 for simplicity
	* Allowing UTF16 and UTF32 would allow faster storage in some cases, BUT:
	* Would require BOM or similar to distinguish between them (size overhead)
	* Would make readers more complex (must be ready to convert from UTF16LE UTF16BE UTF32LE UTF32BE)
* Context-insensitive codes, all in one byte
	* Not all codes are valid in all contexts, which enables sanity checks.
* No integers larger than 64 bit
	* Though big-int support would be nice, it would make encoders/decoders more complex



# Appendix: Why binary?

### Faster:
* No escaping/unescaping of strings and data
* No base64-encoding/decoding of data
* No decimal conversion of integers and floats
* Big blobs of data can be copied right into place (both ways)

### More secure
* Strings and data do not need to be escaped to be secure (assuming length preamble to all fields, which a text format can also have, but seldom do).
	* Escaping correctly is hard. Which special characters must be removed for a key to be acceptable in XML? Quite a few. This makes it easy to think you have covered all the cases when you haven’t - a false sense of security.
	* Prepending the correct size is easy, and it is very hard to do it wrong while thinking you did it right.
* The added complexity of a binary format encourages using a library for encoding/decoding, meaning the user won’t be tempted to roll his own security holes.

### More compact
* The number 1234567890 require only 4 bytes. QED.

### More precise
* Floating point numbers are restored bit-perfect - this is hard using text formats. Try printing a denormal float in a way that will restore it bit-perfect. I dare you!

### Downsides with binary:
* Difficult for a human to quickly inspect a binary file (without converting it to text first - hence bon2json)
* Much harder to understand a random binary file than a random text file. A text file format can often be understood from simply looking at it.
	* So much more important for all binary formats to conform to a standard key-value-pair system - hence BON.



# Appendix: Why a property tree (nested key-value pairs)?

* Makes the format and its use self-documenting
* Makes it easy to extend the format with new key:s which can safely be ignored by older (or uninterested) readers



# FORMAT TODO
* Specially compressed array types:
	* Bool - many true/false values ?
	* Bits   - many ones/zeros ?
* Blocks - powerful enough for most usages?



# Implementation TODO
* Blocks
* Compressed integers/strings/arrays/structs/block-refs
* Memory management
	* A lot of mallocs, no free:s
		
	

# BNF-ish (with regex syntax) - INCOMPLETE

Words in capitals are control byte values (atoms). Explanations within bracketed, e.g. VLQ[size]


	document		::= header? doc_contents footer?
	header	  		::= BON0
	footer	  		::= ***TODO***
	doc_contents	::= value
					  | block_doc
	block_doc		::= block_decl+
	block_decl		::= BLOCK_BEGIN VLQ[id] VLQ[byteSize] value BLOCK_END
	block_ref		::= FixBlockRef
					  | BLOCK_REF VLQ[id]
	value	   		::= object
					  | list
					  | string
					  | TRUE
					  | FALSE
					  | NIL
					  | fixed
					  | block_ref
	object	  		::= OBJ_BEGIN  (key value)*  OBJ_END
	list			::= LIST_BEGIN  value*  LIST_END
	key		 		::= string
					  | block_ref
	sized_string	::= FixString
					  | STRING_VLQ VLQ[nbytes]
	string	  		::= sized_string byte* 0x00

	fixed			::= int
					  | float
					  | struct
					  | array

	int				::= PosFixInt
					  | NegFixInt
					  | int_type int_value

	int_value		::= byte+

	int_type		::| SINT8	 
					  | UINT8	 
					  | SINT16_LE 
					  | UINT16_LE 
					  | SINT16_BE 
					  | UINT16_BE
					  | SINT32_LE
					  | UINT32_LE
					  | SINT32_BE
					  | UINT32_BE
					  | SINT64_LE
					  | UINT64_LE
					  | SINT64_BE
					  | UINT64_BE

	float			::= float_type float_value

	float_value	  	::= byte+

	float_type		::= FLOAT32_LE 
					  | FLOAT32_BE 
					  | FLOAT64_LE 
					  | FLOAT64_BE 

	fixed_type		::= int_type
				 	  | float_type
				 	  | struct_type
				 	  | array_type

	fixed_value 	::= int_value
				 	  | float_value
					  | struct_value
					  | array_value

	struct			::= struct_type struct_value
	sized_struct	::= FixStruct
					  | STRUCT_VLQ VLQ
	struct_type		::= sized_struct (key fixed_type)*
	struct_value	::= fixed_value*

	array			::= array_type array_value
	sized_array		::= FixArray
					  | ARRAY_VLQ VLQ
	array_type		::= sized_array fixed_type
					  | FixByteArray
	array_value		::= fixed_value*



# VLQ
There are a few places in the BON file format when we want to encode sizes. The size of a string or and array can be huge and would in some cases require up to 64 bits to encode. However, most are much smaller than this and so always using 64 bits is a waste of space.

BON solves this by using encoding these values using [Variable Length Quantity](http://en.wikipedia.org/wiki/Variable-length_quantity), or VLQ for short. VLQ:s are used for string and array sizes, as well as block ID:s. They are unsigned integers that are generally small, and must be strictly smaller than 2^64 (requiring at most 10 bytes to encode).

The VLQ format naturally solves endian issues, and [implementations abound](http://rosettacode.org/wiki/Variable-length_quantity).

Though one can encode small numbers with a small number of bytes using VLQ, nothing forbids an encoder to waste many bytes to encode small numbers, while still adhering to the VLQ format. For instance, the value 42 could be encoded as any of:

	0x2A
	0x80 0x2A
	0x80 0x80 0x2A
	…etc

This can be helpful when an encoder doesn't want to fill in the size of something right away. In particular, when encoding a block the encoder could initially fill in the block size as:

	0x80 0x80 0x80 0x80 0x80 0x80 0x80 0x80 0x80 0x00
	
The block can then be written and once done, and we know how large the block ended up becoming, the encoder could go back and change that VLQ size to any value (less than 2^64)