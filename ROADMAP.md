
# Roadmap

Feature todo list in order:

- strict ownership only + new ref type '&', refs can only be stored in variables
- >&?MyClass | new type syntax
- do not allow borrow and '&' in generic types

- enums
- Only allow local variables as a borrowed argument in a function call
- compare errors
- swap token
- package manager
- 'each' declarations in structs/classes, 'each chars: each_chars_init, each_chars;' | usage: 'each mystr.chars as char {...}'
- testing features
- install global packages + "bin" setting in ki.json to place binaries in ~/.ki/bin
- access type 'namespace'
- declare functions for classes/structs from other libraries
- trait sub-types (same syntax as generics)
- multi-threaded AST parsing + IR gen
- inline if ... ? ... : ...
- custom allocators | MyClass{...} @my_alloc
- --watch param to watch for file changes and trigger a build
- vscode extension
- closure functions
- cross compiling
- async library
- calling functions using named arguments
- implement IO URing where useful
- replace malloc with something better
- __leave_scope (code executed when a variable leaves its scope)

## Maybe

- interfaces | useful, but we'll have to introduce virtual functions which we try to avoid

## Done

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
