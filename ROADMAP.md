
# Roadmap

Feature todo list in order:

- format string
- dont allow class_pa of strict type values, swap token
- Calling functions using named arguments
- Package manager
- Testing features
- access type 'namespace'
- Declare functions for classes/structs from other libraries
- Multi-threaded AST parsing + IR gen
- inline if ... ? ... : ...
- Custom allocators | MyClass{...} @my_alloc
- --watch param to watch for file changes
- vscode extension
- Async library
- Cross compiling
- __leave_scope (code executed when a variable leaves its scope)
- Anonymous functions

## Maybe

- Interfaces | they are kind of pointless because we dont use virtual functions and therefore cant use them as types

## Done

- __eq
- @vs { ... } (value-scopes)
- __before_free
- Strict ownership types
- Shared ownership types
- Threading
- Check class property access types (public,private,readonly)
- Custom iterators
- Define which arguments need to take ownership
- Error handling
- Ownership algorithm
- Generics
- Traits
- ?? | ?!
