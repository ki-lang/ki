
# Notes

## Class

Class {
	name: string,
	is_struct_pointer: bool
	struct: ?Struct {
		props
	},
	type: type,
}

## Class defs

class X {}
c parser: struct Y { ... }
class ctype Y1 struct Y* {} // Check if type exists 
class ctype Y2 struct Y {} // Check if type exists

## Class Init

X { ... }
c:Y { ... }
c:Y* { ... }
Y1 { ... }
Y2 { ... }

## Compiler globals

//classes[pkg:ns:X] = class
c_classes[pkg__ns__X*] = class
structs[pkg__ns__X] = class

structs[pkg__ns__Y] = class

//classes[pkg:ns:Y1] = class
c_classes[pkg__ns__Y*] = class

//classes[pkg:ns:Y2] = class
c_classes[pkg__ns__Y] = class

## Init return types

X { ... } => Type { pointer: true, class: X, pointer_of: Type { struct: X } }
c:Y { ... } => Type { class: Y2, struct: Y }
c:Y* { ... } => Type { class: Y1, struct: Y }
c:Y1 { ... } => Type { pointer: true, class: Y1, pointer_of: Type { struct: Y } }
c:Y2 { ... } => Type { class: Y2, struct: Y }

