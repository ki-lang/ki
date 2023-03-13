
# Race conditions

Ownership type: *MyClass
shared token: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use


# Ref counting

- Track uses of variables (logic per scope)
-- upref at start of scope by number of uses until end of scope or mutation of the variable
-- if mutating a variable, setup a new upref slot with count 0
-- if mutating a variable, deref previous value
-- if uses 0 deref at end of scope
-- if uses 1 do nothing (ownership got passed)
-- if uses > 1 deref at end of scope

```
let x = MyObject{...}; # initialized with rc 1
# upref slot (+4 = 5)
myfunc(x)
let a = x;
let b = x;
if(50/50) {
	# upref slot (+1 = 6)
	let o = myfunc(x).y; deref return value of func because chained of prop access

	#o--; // unused
}
let c = x;

# we do not deref 'x' because it gave away ownership
// myfunc will deref x somwhere (5)
// myfunc will deref x somwhere (4)
# a-- // uses == 0 (3)
# b-- // uses == 0 (2)
# c-- // uses == 0 (1)
# x-- // uses > 1 (0) -> free
```
