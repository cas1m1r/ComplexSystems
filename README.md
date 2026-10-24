# Understanding Complex Systems — expanded textbook, revision 0.3

Open `understanding-complex-systems.html` for the self-contained reading copy, or `understanding-complex-systems.md` for portable source text. Both include all teaching listings and 27 chapters. The offline reader has no external asset dependencies.

`originals/` contains the 16 uploaded C files byte-for-byte. `examples/` contains 22 teaching translation units: 21 standalone programs and one object/assembly probe. `sources/` preserves the original outline, older supplied checkpoint, conversation captures, and uploaded-code hashes. `validation/` records compile diagnostics for the original files and new checks of the teaching versions.

Build on Linux/WSL with `make`. Do not infer correctness from running the intentionally undefined counters. Consult the relevant chapter before running a deliberately flawed logical example. The newly supplied `poll_ping` is transport-only and binds to loopback; peer voting and replicated commit remain open coursework.

`continuation-checkpoint.json` identifies the last included C160 exchange, about poll-based socket handling. No new election or Byzantine commit implementation is claimed. No public release or license has been selected.
