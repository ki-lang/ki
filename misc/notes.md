
# Race conditions

strict ownership type: *MyClass
shared ownership type: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use
strict ownership type cannot be used in:


# Ref counting

- We keep track of variable usage via usage lines
- When we redeclare a variable, or reach a scope endpoint (return,break,continue), we end the usage line
-- The end of a scope is also an endpoint for the variables that were declared in that scope
- logic for ending an usage line:
-- if has ancestors, traverse ancestors and call end_usage_line on them
-- else if has a ref token, disable token
-- else add deref to usage line scope

Simply. When we reach the end of a variable, we disable the last upref, if there is no upref, we check if there are ancestors
if so, we apply the logic to the ancestors instead, if there are no ancestors, we deref in the current scope

-- end of line
if upref, disable
if ancestors {
  each end_of_line(ancestor)
  if ul->clone_from
    bool all_deref = true;
    each ancestors
       if(!oldest->deref_token) all_deref = false; break;
    if all_deref
       disable all deref tokens from oldest ancestors
       end_of_line(ul->clone_from);
}
else add deref


# TODO

- on assign, store right->type->nullable in the usage line, and use this in the type check (also do this with declares)

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
