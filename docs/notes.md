
# Notes

## Todo

- replace nx_json with cJSON
- const types
- same 'or' parser for fcalls and ifnull
- check if && and || is handled correctly e.g. if(true || myfunc()) should not execute myfunc

## Compile cache

recompile if:
- file changed
-> recompile dependency_for array

when to add to depency_for:
- when using class from other file
- when using a function from other file
- when using enum from other file
