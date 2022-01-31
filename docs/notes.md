
# Notes

## Todo

- type checking on assign
- refcounting + free
- specify list of non-allowed variable/func/class names
- when using throw check function can_error
- concurrency


## Compile cache

recompile if:
- file changed
-> recompile dependency_for array

when to add to depency_for:
- when using class from other file
- when using a function from other file
- when using enum from other file

## RC

a = new A; rf=1

b->something = a; rf=2
b->something = NULL; rf=1;

A* var = a; nothing
b->something = var; rf=2;

free(b); rf=1
free(a); rf=0
return;
