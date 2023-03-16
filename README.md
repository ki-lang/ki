
<div align="center">
<p>
    <img width="120" src="https://raw.githubusercontent.com/ki-lang/ki/master/misc/logo-edges.png">
</p>
</div>

# Ki

[Website](https://ki-lang.dev) | [Documentation](https://ki-lang.dev/docs)

(We are still in alpha, some things might change)

ki is a programming language designed to be super fast and easy to use. You can choose between manual memory management (structs) or reference counted objects (classes (no OOP)). The compiler keeps track of ownership and uses reference counting as a fallback. Meaning if you write your code like rust, it runs like rust. But if you want to use an object twice, just do it and it will work. We also offer strict ownership which prevent you from doing this.

Goals: fast run time, fast compile times, simplicity and great package management.

## Basic usage

```bash
# ki -h for help
ki build src/*.ki -o hello
./hello
```

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm && link --force llvm`

Linux: `sudo apt install llvm`

```bash
git clone git@github.com:ki-lang/ki.git
cd ki
make
```

## Build from source (Windows)

Note: Windows still needs some work. For now u should compile with minGW. It might compile but might still segfault when building code. It's LLVM problem.

install msys2 & open the mingw terminal

```bash
pacman -S --needed git wget mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make mingw-w64-x86_64-python3 autoconf libtool
pacman -S mingw-w64-x86_64-llvm
pacman -S mingw-w64-x86_64-clang
make
```

## Language overview

```
use {namespace};

header "{path}";
link "{lib-name}";

global/shared_global {type} {name};

func ({type1} {arg1-name}, {type2} {arg2-name}) {return-type} [or err1,err2] {...code...}

enum {name} {
	{item1},
	{item2},
	{item3}: {int-value},
}

class {name} {
	public|private|readonly {type} {prop-name} [ = {default-value}];
	public|private [static] func {name}(...args...) {return-type} {...code...};
	trait {name};
}

trait {name} {
	... same as class ...
}
```

Tokens

```
let [mut] {var-name} [: {type}] = {value};

if {value} { ...code... } 
else if {value} { ...code... } 
else { ...code... }

while {value} { ...code... }

each {value} as v/k,v { ...code... }

break;
continue;

return {value};
throw {error-name};
exit {int-value};
panic {string-value};

```

Values

```
"..." (string)
f"..." (format-string)
'\n' (char|u8)
true/false
null
5 (int)
5.123 (float)
MyClass { prop1: {value}, prop2: {value} };
let myfuncref = myfunc; (function reference)

__EXE__ (executable path)
__DIR__ (executable dir)
sizeof({type})
stack_alloc({int})

getptr {var-name}
ptrv {ptr-value} as {type}; (reads ptr address as {type})
ptrv {ptr-value} as {type} = {value}; (same as setptrv)

atomicop {var-name} ADD|SUB|AND|OR|XOR {int};
let x = { return 1; }; (value scope)

let task = async {func-call}; (call function async)
... = await task; (wait for return value)
```

Types

```
i8,i16,i32,i64,u8,u16,u32,u64,ixx,uxx (integers)
fxx (float)
bool
String
Array<T>
Map<T>
ptr (address number aka. pointer)
```

