
# Notes

## Compile cache

-> scan funcs/classes
-> if(changed) scan prop/glob values   -> look for used funcs/classes
-> if(changed) scan ast                -> look for used funcs/classes
-> if(changed) write-c                 -> store per cname used cnames in cache
-> if(changed) compile o-file
-> loop g_main_func used cnames
-> if(used) add to o_files

---

recompile if:
- file changed
-> recompile dependency_for array

when to add to depency_for:
- when using class from other file
- when using a function from other file
- when using enum from other file