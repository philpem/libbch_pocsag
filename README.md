# libbch_pocsag

libbch_pocsag is a simple BCH(31,21)+Parity (POCSAG) error correction coding and decoding library. The code is built into a single C source file for ease of use.

It implements the Bose–Chaudhuri–Hocquenghem code (plus even parity) scheme. This is the same as that used by the POCSAG radio paging protocol for error correction coding.
The encoder is iterative, with a Meggitt-type decoder.

libbch_pocsag includes a test harness which checks a few known test vectors (from the ITU-R RPC No.1 / Rec.584 standard), all possible single-bit errors, and all possible double-bit errors. This ensures that the BCH algorithm core is basically functional, and capable of correcting up to two erroneous bits.

Doxygen-style comments are included to show how the library is intended to be used.

This is part of a personal investigation into radio paging protocols, and is released under the Apache License version 2.0 for use by the amateur radio community.

Bug reports and pull requests are welcome!

73's de Phil M0OFX.
