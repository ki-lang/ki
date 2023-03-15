
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:


# Ref counting

UsageLine {
	Chunk* first_move
	UsageLine* follow_up
	int moves_max              // max(previous)
	int moves_min              // lowest(previous)
	int reads_after_move
}

Rules:
- every declare create a Decl and a UsageLine
- every assign creates a new UsageLine in the current scope and saves it in scope->usage_lines[decl]
-- if the scope already has a usage line, we replace it
- at the end of each scope we merge the usage line with the parent if the scope did not return
-- current->moves_max = max(current->moves_max, child->moves_max)
-- current->moves_min = min(current->moves_min, child->moves_min)
-- current->first_move = current->first_move ?? child->first_move;

- if/else/??/?!
-- clone the current usage line into each scopes, after parsing a scope set follow up as current if it didnt return
-- after all if/else scopes have been parsed, we loop over the clones and merge the clone with the current
-- then we loop over all scopes again and merge with the last line (if the scope did not return)
- if there is no else scope we must generate one, but only if moves_max = 1 and moves_min = 0

- a 'move' means every time we pass it as a func argument or use it as the right side value in an assign/declare
-- a move will increase both moves_max and moves_min

# TODO

- prop access | deref on value instantly when on value is func call or class init

- type check internal funcs : __ref __deref ...
- exit & panic
