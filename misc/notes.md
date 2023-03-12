
# Race conditions

Ownership type: *MyClass
shared token: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use


# Ref counting

```
let x = MyObject{...}; # initialized with rc 1
# upref slot (+4 = 5)
myfunc(x) ++ (2)
let a = x; ++ (3)
let b = x; ++ (4)
if(50/50) {
	# upref slot (+1 = 6)
	let o = myfunc(x).y; ++ (5) && deref return value of func because chained of prop access

	#o--; // unused
}
let c = x; ++ (6)

# we do not deref 'x' because it gave away ownership
// myfunc will deref x somwhere (5)
// myfunc will deref x somwhere (4)
# a-- // used == 0 (3)
# b-- // used == 0 (2)
# c-- // used == 0 (1)
# x-- // used >= 2 (0)
```



let x = MyObject{...}; # initialized with rc 1
let z = x;
let a = z; ++ (2)
let b = z; ++ (3)
if(true) {
	b = z; ++ (4)
	b-- (3)
}

z--; (2)
a--; (1) // unused
b--; (0) // unused