
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:
- uncertain scopes, such as if statement
- inside loops


# Ref counting

Rules:
- every declare create a Decl and a UsageLine
- every assign creates a new UsageLine in the current scope and adds it to scope->usage_lines[decl] (return an array of usage lines for that decl)
- at the end of each scope we pass the usage lines to the parent if the scope did not return
- on a return (including break/continue) we loop over the usage-lines of each decl and make conclusions (usage_line->ownership = true/false)
- when making a conclusion, we can make seperrate conclusions per line for non-mut decls, for mut decls, we must group them
- when a decl is used, we find the first scope that has usage lines for that decl and update that set of lines
- when updating, if the usage lines are from the current scope, we add +1 to ->uses, if from a parent scope, we do +1 on ->max_uses
- when a decl is used, we replace the value with a v_upref_multi_use, struct: UpRefMultiUse{ Value*, UsageLine* }
-- This value will only generate upref IR instructions if the usage line says it has ownership


# TODO

- prop access | deref on value instantly when on value is func call or class init
- When using variables in recurrent pieces of code, decl->uses += 2; instead of += 1;

- type check internal funcs : __ref __deref ...
- exit & panic
