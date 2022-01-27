
# Notes

## Todo

- traits
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
