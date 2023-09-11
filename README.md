
<div align="center">
<p>
    <img width="120" src="https://raw.githubusercontent.com/ki-lang/ki/master/misc/ki-logo-circle.png">
</p>
</div>

# Ki (Discontinued)

We decided not to continue the development of ki because of how the programming world works. Ki was built to be a language that had all the good stuff, but we never saw memory management as an obstical for this ideal language. We wanted automatic memory management without garbage collection and no memory leaks. Turns out, the only way to do this, is using the ownership model. Which we did and everything worked perfect. But ownership comes with complexity and we realised we were going to be too similar to Rust. Which was fine, but.. After looking at many online discussions, we understood that even Rust has problems with making people adopt the language. We know it's popular, but how many companies are using it? Very few. So we came to the realisation that if we are too similar to Rust, the chance of ki being used in the real world would almost be zero. It seems like companies or developers in general prefer to use the language that they already know, even if that language has a mountain of problems. Therefor we decided it would be better for us to spend our energy on something else.

That being said, if we could ever find a new memory management technique that has all our requirements while not introducing complexity, we would certainly look into starting a new project. It would have such an edge over other languages that people might get interested. Any how. This repository will remain public for the people who wish to know how a compiler is built.

---

[Website](https://ki-lang.dev) | [Documentation](https://ki-lang.dev/docs) | [Discord](https://discord.gg/T7pR6fm6SC)

(We are still in alpha, the standard lib still needs alot of work)

ki is a type safe compiled language designed to be fast and easy to use. It does not have any garbage collection and instead uses ownership combined with minimal ref counting to manage memory. Alternatively you can manage your own memory using 'struct' instead of 'class'. We also allow you to compile from any platform to any platform out-of-the-box. We have generics. We have 'null' but runtime null errors do not exist. We have an awesome way to return/handle errors. Our compile times are much faster than other language (and we havent optimized it yet). We use LLVM as a back-end, so all your release code will be super optimized. We are also working on a fast/simple/versioned package manager.

Goals: fast run / compile times ⚡ simplicity and great package management 📦

## Compatibility

|        | MacOS | Linux | Windows |
| ------ | ----- | ----- | ------- |
| x86_64 | ✔     | ✔    | ✔       |
| arm64  | ✔     | WIP   | WIP     |

## How to install

```
curl -s https://ki-lang.dev/dist/install.sh | bash -s latest
```

More info:  [Install docs](https://ki-lang.dev/docs/dev/install)

## Basic usage

```bash
# ki -h for help
ki build src/*.ki -o hello
./hello
```

## Example code

```js
fn main() void {
    println("Hello world!");
}
```

## Cross compiling

| Building for     | From macOS x86_64  | From macOS arm64     | From Linux x86_64  | From Linux aarch64  | From Windows x86_64 | From Windows aarch64 |
| ---------------- | ------------------ | -------------------- | ------------------ | ------------------- | ------------------- | -------------------- |
| macOS x86_64     | ✅                 | ✔️                   | ✅                 |🏃                  |✅                   | 🏃                  |
| macOS arm64      | ✅                 | ✔️                   | ✅                 |🏃                  |✅                   | 🏃                  |
| Linux x86_64     | ✅                 | ✔️                   | ✅                 |🏃                  |✅                   | 🏃                  |
| Linux aarch64    | 🏃                 | 🏃                   | 🏃                 |🏃                  |🏃                   | 🏃                  |
| Windows x86_64   | ✅                 | ✔️                   | ✅                 |🏃                  |✅                   | 🏃                  |
| Windows aarch64  | 🏃                 | 🏃                   | 🏃                 |🏃                  |🏃                   | 🏃                  |

- ✅ Tested and verified via CI(All cross-compilations do not verify by CI that they can run in target system, only that they can build).
- ✔️ Should work, but not tested in CI.
- 🏃 WIP

## Build from source (Linux / macOS / WSL)

macOS: `brew install llvm@15 && brew link llvm@15`

Ubuntu(Debian): `sudo apt-get install llvm-15 clang-15 lld libcurl4-openssl-dev`

```bash
git clone https://github.com/ki-lang/ki.git
cd ki
make
```

## Build from source (Windows)

You can build it natively, but you need the LLVM modules which LLVM does not provide with their installer. We do have our own LLVM builds which you can find in the dist/toolchains.sh script. If you want a simple setup, you can use msys2 (mingw) setup.

install msys2 & open the terminal

```bash
pacman -S curl make mingw-w64-x86_64-cmake mingw-w64-x86_64-llvm mingw-w64-x86_64-clang mingw-w64-x86_64-lld

git clone https://github.com/ki-lang/ki.git
cd ki
make
```
