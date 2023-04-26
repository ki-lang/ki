
# Building distributions

## Linux from Linux

```
sudo apt install libkrb5-dev libnghttp2-dev libidn2-dev librtmp-dev libssh-dev libpsl-dev libdap-dev libbrotli-dev libldap2-dev
```

```
make linux_dist_from_linux
```

## Macos from Linux

Setup

```
Download macOS SDK 11.3 : https://github.com/phracker/MacOSX-SDKs/releases
Unpack in dist directory and rename to macos-sdk-11-3
Download LLVM 14 for apple. e.g. https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-x86_64-apple-darwin21.0.tar.xz
Unpack in dist directory and rename to macos-llvm-14
```

Then, in the project root directory, run:

```
make macos_dist_from_linux
```
