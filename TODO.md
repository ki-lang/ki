
# Todo

- replace nx_json with cJSON
- const types (maybe)
- check if && and || is handled correctly e.g. if(true || myfunc()) should not execute myfunc
- dont allow class instances in async calls
- utf8 support | keep strings as byte arrays | convert to runes for substr,indexof,char and convert back
- http SSL
- websockets (+ websockets secure)
- signals
- handle read/write errors e.g. EAGAIN
- libvips for image manipulation
- cross platform GUI library using SDL, GLFW, SFML or GLUT
- allow compiling with clang & cross compilation with clang
- private & readonly checks
- allow ifnull/notnull on func args
- ifnull 'do' scope + else scope
- notnull else scope
- remove namespaces from headers and define all definitions in the fc where include is used
- check multiple vars in ifnull
- build tests

## Future

- switch from gcc to LLVM : no more need for a c compiler
