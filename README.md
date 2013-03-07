BON - Binary Object Notation
============================

A binary serialization format
-----------------------------
EARLY DRAFT
-----------------------------


# What?
BON is a binary format for serializing data for storage or communication. It is in many ways a binary analog to JSON, and attempts to remedy the shortcomings of other formats, such as BSON and MessagePack, while still being simple, unlike more advanced formats such as HDF5.

# MessagePack
BON is highly influenced by MessagePack, and it might be worth to read the MessagePack format specification before reading the BON specification. BON attempts to remedy the following limitations of MessagePack:

* No self-identifying header
	* A four byte magic header identifies BON - no such luck for MessagePack.
* Bad unicode support
	* In MessagePack there is not string type, only a byte data type, for which one cannot assume an encoding (UTF8, UTF16 etc).
* Slow on big endian machines
	* One has to convert to little endian in MessagePack - BON handles big and little endian natively.
* No support for large data blocks
	* In MessagePack you cannot write a million floating points numbers in an efficient way - one will have to insert a one-byte float header for each number.
	* In BON one can write the same data as as homogenous array with zero overhead.
* No support for streaming
	* One cannot stream a MessagePack list since any such list much has the size set first. BON supports full streaming of lists and objects/maps.