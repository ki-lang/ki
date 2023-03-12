
# Race conditions

Ownership type: *MyClass
shared token: **MyClass

only these types can be shared with other threads or globals
All it's properties must also be owned or shared types

shared types must be captured before use


# Ref counting

```
let x = MyObject{...}; # initialized with rc 1
# x.rc += 3 // gives away ownership 3 times in this scope
myfunc(x)
let a = x;
let b = x;
if(50/50) {
	# x.rc += 1 // gives away ownership once in this scope
	myfunc(x);
}
let c = x;

# we do not deref 'x' because it gave away ownership
// myfunc will deref x somwhere because it doesnt return ownership (3)
# a.rc-- == 0 : free(a); (2)
# b.rc-- == 0 : free(b); (1)
# c.rc-- == 0 : free(c); (0)
```
