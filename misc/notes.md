
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:


# Ref counting

Rules:
- every declare create a Decl and a UsageLine
- every assign creates a new UsageLine in the current scope and adds it to scope->usage_lines[decl] (return an array of usage lines for that decl)
-- if the scope already has usage lines, we create a new array
- at the end of each scope we pass the usage lines to the parent if the scope did not return
- on a return (including break/continue) we loop over the usage-lines of each decl and make conclusions (usage_line->ownership = true/false)
-- ->ownership = (uses == 1 && max_uses == 1)
- when a decl is used, we find the first scope that has usage lines for that decl and update that set of lines
- when updating, if the usage lines are from the current scope, we add +1 to ->uses, if from a parent scope, we do +1 on ->max_uses

- Decl use means every time we access the variable
-- If the current uses & max uses is 0 and the variable access is not inside a func arg or assign val, we should ignore it
- When a decl is used as a fcall argument or the value of an assign, we must upref the decl
-- It must only upref when it doesnt have ownership, so we use v_upref_multi_use, struct: UpRefMultiUse{ Value*, UsageLines* }
-- This value will only generate upref IR instructions if the usage line says it has ownership

- if/else:
-- todo: see if we can generate derefs in order to equalize the uses in each if/else scope

# TODO

- prop access | deref on value instantly when on value is func call or class init
- When using variables in recurrent pieces of code, decl->uses += 2; instead of += 1;

- type check internal funcs : __ref __deref ...
- exit & panic
