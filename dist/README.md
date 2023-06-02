
# Building distributions

Building distributions should be possible on linux and macos. However, we have not tested macos.

Our make files will download & setup toolchains for all targets, no system libraries are required. You will need clang & lld to build.

## Step 1

Install make, clang and lld on your local system

## Step 2

in the project root, run: `make dist_setup` (this will take a while)

## Step 3

in the project root, run: `make dist_all`

or you can build individual distributions

```
make dist_linux_x64
make dist_macos_x64
make dist_macos_arm64
make dist_win_x64
```
