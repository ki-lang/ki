
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

## How to install

```
curl -s https://ki-lang.dev/dist/install.sh | bash -s latest
```

## Basic usage

```bash
# ki -h for help
ki build src/*.ki -o hello
./hello
```

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm-15 && link --force llvm`

Linux: install `llvm-15`, see: [apt.llvm.org](https://apt.llvm.org/)

```bash
git clone git@github.com:ki-lang/ki.git
cd ki
make {linux|macos}
```

## Build from source (Windows)

Note: Windows still needs some work. For now u should compile with minGW. It might compile but might still segfault when building code. It's a LLVM problem.

install msys2 & open the mingw terminal

```bash
pacman -S --needed git wget mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make mingw-w64-x86_64-python3 autoconf libtool
pacman -S mingw-w64-x86_64-llvm
pacman -S mingw-w64-x86_64-clang
make
```
