
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

Strategy:
- init object with rc = 0
- Every time a value that's a class instance with refcounting:
-> rc-- it's current value before assign + check if rc = 0, if so delete
-> rc++ after the assign
- How to return:
-> rc++ the return value and store it in _KI_RET
-> rc-- all local variables except _KI_RET
-> return _KI_RET;

// Example
A* a = something(); rc = 1
B* b = createB();
b->prop = a; rc = 2
b->prop = NULL; rc = 0; -> we cannot dealloc here because a still holds a reference

// deref a, so rc = 1;
// deref b, so rc = 0;
return;
