# Doxygen documentation

This directory contains the G4Cosmic Doxygen overview page and Doxygen input template.

Build the API documentation with:

```bash
cmake -S . -B build -DG4COSMIC_BUILD_DOCS=ON
cmake --build build --target g4cosmic_docs
```

The generated HTML is written under:

```text
build/docs/doxygen/html/
```
