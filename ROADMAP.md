
# Roadmap

Feature todo list in order:

- immutability so one can borrow properties while knowing they wont go out of scope
- trait sub-types (same syntax as generics)
- readonly types
- borrow_each
- compare errors
- dont allow class_pa|array_item of strict type values, swap token
- Calling functions using named arguments
- package manager
- testing features
- do not allow globals in a fcall argument that borrows the value
- install global packages + "bin" setting in ki.json to place binaries in ~/.ki/bin
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

- automatically check if vars are mutated or not to know if we need to alloc on the stack or not
- isset keyword
- format string
- http server
- __eq
- @v { ... } (value-scopes)
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
