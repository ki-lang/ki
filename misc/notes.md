
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:


# Ref counting

UsageLine {
	Scope* init_scope          // Used to get context inside loops
	Chunk* first_move          // Used for error messages for strict ownership
	UsageLine* follow_up
	int moves_min              // lowest(previous)
	int moves_max              // max(previous)
	int reads_after_move
}

Notation:
- 0,1 means: min 0 moves and max 1 move
- The logic below is focussed on a single declaration, logic must be applied for each declaration

Rules:
- every declare create a Decl and a UsageLine
- every assign creates a new UsageLine in the current scope and saves it in scope->usage_lines[decl]
-- if the scope already has a usage line, we replace it
- at the end of each scope we merge the usage line with the parent if the scope did not return
-- current->moves_min = min(current->moves_min, child->moves_min)
-- current->moves_max = max(current->moves_max, child->moves_max)
-- current->first_move = current->first_move ?? child->first_move;

- if/else/??/?!
- parse all scopes
- get min max from scopes
-- max: 0, do nothing
-- max: 1, min: 1, do nothing
-- max: 1, min: 0, all derefs to scopes with 0 uses
-- max: 2+, do nothing
- update left line uses +max


- left ?!/?? right, we must generate an 'else' if the clone line is a 0,1
```
%0 = left
if %0 == null {
	%0 = right
} <-- generate else if 0,1 usage
```

- while
-- clone usage line, merge with current line after
-- after loop also merge with last line (if mutable decl)

- a 'move' means every time we pass it as a func argument or use it as the right side value in an assign/declare
-- a move will increase both moves_max and moves_min
-- a move in side a loop scope we must add +2 if the usage line starts outside the loop

- return/break/continue
-- for each decl we create a token tkn_deref_unless_single_use and attach the value and usage line

# TODO

- prop access | deref on value instantly when on value is func call or class init

- type check internal funcs : __ref __deref ...
- exit & panic


# Future readme

## Install

Linux / macOS / WSL:
```bash
wget -O - https://ki-lang.dev/dist/install.sh | bash -s dev
```

Windows
```
Not supported yet, working on it
```

Uninstall
```bash
sudo rm -f /usr/local/bin/ki
sudo rm -rf /opt/ki/{version}
```

## Http server example

```
use ki:http;

func handler(http:Request req) http:Response {
	return http:Response.html("Hello from example");
}

func main() i32 {

	let s = http:Server.init({ port: 9000, handler: handler }) or {
		println("Failed to initialize http server");
		return 1;
	};
	s.start();

	return 0;
}
```
