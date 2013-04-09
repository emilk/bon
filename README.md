BON - Binary Object Notation
============================
BON  is a fast, flexible, general purpose binary serialization format.

***Key features:***

* Fully JSON compatible (both ways)
* Zero-overhead read and write of large amounts of data (e.g. an array of floats).
* Self-identifying header
* Always Unicode
* Supports streaming
* Supports quick browsing of huge files
* Optional CRC-32 for every file

[Read more about the BON format here](https://github.com/emilk/bon/wiki/BON-format).


### What's wrong with MessagePack/BSON/protobuffers/...?
They all fail in one or more of the key features listed above.

### I like it - can I help?
Please do! The BON format is still work in progress, and so I'm looking for any and all feedback.

The code is all open source (under the MIT license) and implementations in other languages is most welcome.

# Implementation
I have written a C library implementation of BON which you can [read about here](https://github.com/emilk/bon/wiki/Lib).

In addition, I have written [tools for converting between BON and JSON](https://github.com/emilk/bon/wiki/bon2json-and-json2bon).