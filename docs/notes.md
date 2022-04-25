
# Notes

## Todo

- remove static & threaded tokens
- add threaded_global & unsafe_global
- allow ifnull & notnull on globals
- rename notnull to ifvalue
- clear memory allocating funcs in mem allocator

## Compile cache

recompile if:
- file changed
-> recompile dependency_for array

when to add to depency_for:
- when using class from other file
- when using a function from other file
- when using enum from other file
