
<div align="center">
<p>
    <img width="120" src="https://raw.githubusercontent.com/ki-lang/ki/master/misc/logo-edges.png">
</p>
</div>

# Ki

[Website](https://ki-lang.dev) | [Documentation](https://ki-lang.dev/docs) | [Discord](https://discord.gg/T7pR6fm6SC)

(We are still in alpha, the standard lib still needs alot of work)

ki is a type safe compiled language designed to be fast and easy to use. It does not have any garbage collection and instead uses ownership combined with minimal ref counting to manage memory. Alternatively you can manage your own memory using 'struct' instead of 'class'. We also allow you to compile from any platform to any platform out-of-the-box. We have generics. We have 'null' but runtime null errors do not exist. We have an awesome way to return/handle errors. Our compile times are much faster than other language (and we havent optimized it yet). We use LLVM as a back-end, so all your release code will be super optimized. We are also working on a fast/simple/versioned package manager.

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

Linux: install `llvm-15`, Ubuntu/debian see: [apt.llvm.org](https://apt.llvm.org/)

```bash
git clone git@github.com:ki-lang/ki.git
cd ki
make
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
