# C Canonical ABI - A Proof of Concept

This project is a proof-of-concept for a new ABI standard for C and other languages, aiming to solve common interoperability issues.

It consists of:
*   A C library (`libcanonical`) that defines and manages a "canonical" ABI.
*   A tool (`shim_generator`) that parses C headers and generates wrappers to translate between the platform ABI and the canonical ABI.
*   A whitepaper explaining the design and goals.
