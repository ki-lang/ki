
# Todo

- replace nx_json with cJSON
- dont allow class instances in async calls
- websockets (+ websockets secure)
- signals
- handle read/write errors e.g. EAGAIN
- libvips for image manipulation
- cross platform GUI library using SDL, GLFW, SFML or GLUT
- allow compiling with clang & cross compilation with clang
- private & readonly checks
- remove namespaces from headers and set all definitions under a custom namespace and define an alias after "import" to access that namespace
- check multiple vars in ifnull
- object cloning
- date/time library
- mysql library
- const types
- better throw errors
- f"" format string e.g. f"Hello {name}, {msg}"
- integer type/byte checking
- floats
- kifmt
- throw error class with error_code & error_msg
- dont allow getptr on number literals
- foreach on Map<>
- package requirements e.g. ki version between x/y
- macro values via ki build --def "env=production,enable_some_feature=1"
- convert using #{type} -> e.g. #i32 => __to_i32()
- []#String (empty array of strings value), ["hello"] (array of strings value)
- (re-)compile entire namespaces to object files to reduce amount of linker files
- valgrind -> check if there is un-freed memory in the compiled programs

## Later

- more tests
- rewrite c generator
- pre-allocating data
- let compiler generate api-docs for a package,namespace,class,func,enum

## Future

- switch from gcc to LLVM : no more need for a c compiler
