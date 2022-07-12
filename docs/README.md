
# Docs

## Types

```
// Numbers
i8 u8 i16 u16 i32 u32 i64 u64 ixx uxx
@ num = 100; (i32 default);
@ num = 100#u8;
@ num = -20#ixx; // i32 or i64 based on compile target
```

```
// Built-in classes
String Map<T> Array<T>
@ x = "Hello";
@ map = Map<String>{};
map.set("world", x);
@ arr = Array<String>{};
arr.push(x);
```

```
// Other
bool (true/false)
ptr (memory address number, e.g. 0x123230004)
@ b = true;
@ x = ki:mem:alloc(3);
setptrv x to b;
setptrv x+1 to b;
setptrv x+2 to false;
```

## Nullable types
```
?String str = null;
```
Note: only pointer types can be nullable. Basically all types except the number types (i8,u8,...)

How to use nullable types:
```
?String str = null;
@ len = str.length; // Syntax error, cannot ask property on null
```
Solution:
```
?String str = null;
ifnull str set "";
// From this point on, the compiler knows 'str' is not 'null'
// The compiler will see 'str' as String type now, instead of ?String
@ len = str.length;
```

## ifnull / notnull
ifnull & notnull is used to prove to the compiler that a certain variable is not null. `ifnull` is mostly used to transform a `?Type` to a `Type` variable. `notnull myvar do { ... }` is used to create a scope where in the compiler is certain a variable is not null.
```
?String x = ...
notnull x do {
	println(x);
}
ifnull x return ...;
// At this point x cannot be null anymore and notnull is not longer needed
println(x);
```

## ifnull actions

```
ifnull mystr set "default string";
ifnull myvar return ...;
ifnull myvar break;
ifnull myvar continue;
ifnull myvar throw my_error_msg;
ifnull myvar panic my_error_msg;
ifnull myvar exit 1;
```

## Functions

```
func my_func (i32 a, i32 b) i32 {
	return a + b;
}
```

## Return errors

```
func plus (i32 a, i32 b) !i32 {
	if(a == 0) throw a_cannot_be_zero;
	return a + b;
}
func main() i32 {
	@ result = plus(0, 2) or value -1;
}
```

## or ...

If a function throws an error, we handle it by using an `or` token after the function call. `or` uses the same logic as `ifnull`, with the exception of `set` being `value`. So: `or value,return,break,continue,throw,panic,exit`. Instead of using `throw` to throw a new error, you can also use `or pass` to pass on the received error.

## Classes

```
class MyClass {
	public i32 a = 1;
	private i32 b;
	readonly i32 c;

	public static func init() MyClass {
		return MyClass{ b: 1, c: 2 };
	}

	public func ab() i32 { return this.a + this.b; }
}
```

## Traits
```
trait A {
	public i32 a = 1;
}

class B {
	public i32 b = 1;

	trait A;

	public func plus () i32 {
		return this.a + this.b;
	}
}
```
