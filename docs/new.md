
# New version

## Goal
- Dont allow creating c types (because its too hacky)

## Compiler steps
- Scan files for class names and detect namespaces and which .h it wants
- Scan the .h files for types and functions
- Scan enums (because they are types) global and in classes
- Scan global variables & functions
- Scan class properties
- Build ast for all functions
- Write-c

## .h Header imports
- each import has its own parser instance
- keep global identifiers map to lookup for duplicate names
