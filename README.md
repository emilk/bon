BON - Binary Object Notation
============================

# What is BON?
BON stands for "Binary Object Notation", and is a general purpose binary serialization format.

[Read more about the BON format here](https://github.com/emilk/bon/wiki/BON-format).

## Key features
* Fully JSON compatible (both ways)
* Zero-overhead read and write of uniform data (e.g. an array of floats)
* Self-identifying header
* Always Unicode
* Supports streaming
* Supports huge files

# What's wrong with MessagePack/BSON/protobuffers/...?
They all fail in one or more of the key features listed above.

# I like it - can I help?
Please do! The BON format is still work in progress, and so I'm looking for any and all feedback.

The code is all open source (under the MIT license) and implementations in other languages is most welcome.