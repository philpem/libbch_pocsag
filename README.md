# libbch_pocsag

libbch_pocsag is a simple BCH(31,21)+Parity (POCSAG) error correction coding and decoding library. The code is built into a single C source file for ease of use.

It implements the Bose–Chaudhuri–Hocquenghem code (plus even parity) scheme. This is the same as that used by the POCSAG radio paging protocol for error correction coding.
The encoder is iterative, with a Meggitt-type decoder.

Doxygen-style comments are included to show how the library is intended to be used.

## Build-time configuration

The Meggitt decoder needs to recognise 31 "MSB-error" syndromes on each
shift. These are derived from the generator polynomial g(x) = 0x769 — by
linearity, every entry is `x^30 mod g` optionally XORed with `x^k mod g` for
some k in 0..29 — so the table can be (re)generated mechanically rather
than maintained by hand. `tools/gen_meggitt_table.c` is a small standalone
program that prints the bitmap from g(x) if you ever need to regenerate it.

How that table is stored at runtime is selectable via
`-DBCH_MEGGITT_TABLE_MODE=N` at compile time:

| Mode | `N` | Storage | Notes |
| --- | --- | --- | --- |
| `LAZY`   | 0 (default) | 128-byte bitmap in `.bss`, populated on first call from g(x) | Most flexible; smallest source |
| `CONST`  | 1           | 128-byte `static const` bitmap → `.rodata` (ROM-resident on most embedded targets) | Smallest overall footprint |
| `SWITCH` | 2           | No table; a 31-case `switch` statement | Zero data, larger code |

For PC builds the default `LAZY` mode is fine. For embedded targets where
`.rodata` lives in flash, `CONST` is usually the best choice. `SWITCH`
trades data for code and lets the compiler emit a dense decision tree.

## Testing

libbch_pocsag includes a test harness which checks a few known test vectors (from the ITU-R RPC No.1 / Rec.584 standard), all possible single-bit errors, and all possible double-bit errors. This ensures that the BCH algorithm core is basically functional, and capable of correcting up to two erroneous bits.

This is part of a personal investigation into radio paging protocols, and is released under a permissive 3-clause BSD license for use by the amateur radio community (and others).

Bug reports and pull requests are welcome!

73's de Phil M0OFX.
