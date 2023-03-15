
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:


# Ref counting

UsageLine {
	Array* previous
	Chunk* first_move
	UsageLine* follow_up
	int moves_max              // max(previous) add new moves to this (+1 and +2 in loops)
	int moves_min              // lowest(previous)
	int reads_after_move
}

Rules:
- every declare create a Decl and a UsageLine
- every assign creates a new UsageLine in the current scope and saves it in scope->usage_lines[decl]
-- if the scope already has a usage line, we replace it
- at the end of each scope we pass the usage lines to the parent if the scope did not return

- if/else:
-- clone the current usage line into each scopes, after parsing a scope set follow up as current if it didnt return
-- after all if/else scopes have been parsed, we loop over the clones and set the current moves_max to the highest moves_max we find in the sub scopes

# TODO

- prop access | deref on value instantly when on value is func call or class init
- When using variables in recurrent pieces of code, decl->uses += 2; instead of += 1;

- type check internal funcs : __ref __deref ...
- exit & panic
