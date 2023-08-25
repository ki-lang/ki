
# Roadmap

Feature todo list in order:

```
- package platform: pkg.ki-lang.dev
- test if poll frees Listener objects
- fix correctness of shared reference types
- rename '@as' to 'as'

-- release version 0.1.0

- union & c_union
- closure functions
- trait generic types
- function generic types
- break outer loops
- calling functions using named arguments

- add \0 to string data
- c"c-string-constants"

- install global packages that compile the code to a binary in ~/.ki/bin | ki pkg global add {url} | ki pkg global install
- async library

- multi-threaded AST parsing + IR gen
- multiple iterators
- complete debug info
- implement IO URing where useful
```

## Maybe

```
- make ';' optional
- custom allocators | MyClass{...} @my_alloc
- defer token
- interfaces | useful, but we'll have to introduce virtual functions which we try to avoid
```

## Done

```
- inline if ... ? ... : ...
- vscode extension
- language server
- --watch param to watch for file changes and trigger a build
- declare functions for classes/structs from other libraries
- add a message to your errors
- cli option --def
- ki make command
- macros
- array/map shortcuts
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
- {{ ... }} (value-scopes)
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
```
