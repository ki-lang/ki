
# Roadmap

Feature todo list in order:

- compare errors
- dont allow class_pa of strict type values, swap token
- Calling functions using named arguments
- package manager
- testing features
- access type 'namespace'
- declare functions for classes/structs from other libraries
- multi-threaded AST parsing + IR gen
- inline if ... ? ... : ...
- custom allocators | MyClass{...} @my_alloc
- --watch param to watch for file changes
- vscode extension
- cross compiling
- async library
- __leave_scope (code executed when a variable leaves its scope)
- anonymous functions
- implement IO URing where useful
- replace malloc with something better

## Maybe

- interfaces | they are kind of pointless because we dont use virtual functions and therefore cant use them as types

## Done

- isset keyword
- format string
- http server
- __eq
- @vs { ... } (value-scopes)
- __before_free
- strict ownership types
- shared ownership types
- threading
- check class property access types (public,private,readonly)
- custom iterators
- define which arguments need to take ownership
- error handling
- ownership algorithm
- generics
- traits
- ?? | ?!
