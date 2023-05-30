
# Roadmap

Feature todo list in order:

- array/map shortcuts
- closure functions

- unions
- declare functions for classes/structs from other libraries
- install global packages that compile the code to a binary in ~/.ki/bin | ki pkg global add {url} | ki pkg global install
- add a message to your errors

- trait generic types
- function generic types

- access type 'namespace'
- multi-threaded AST parsing + IR gen
- inline if ... ? ... : ...
- --watch param to watch for file changes and trigger a build
- vscode extension
- async library
- multiple iterators
- complete debug info
- calling functions using named arguments
- implement IO URing where useful
- replace malloc with something better

## Maybe

- make ';' optional
- custom allocators | MyClass{...} @my_alloc
- defer token
- interfaces | useful, but we'll have to introduce virtual functions which we try to avoid

## Done

- basic debug info
- testing features
- main argv/argc
- package manager | check name,version in package config + test 'use'
- compare errors
- cross compiling
- swap token
- Rework type system, ownership by default, & for references and * for borrows
- use ':' for single line scopes for if/else/while/each
- automatically check if vars are mutated or not to know if we need to alloc on the stack or not
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
