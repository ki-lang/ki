
(cd "$(dirname "$0")" && clang -emit-llvm -S test.c -o test.ll && cat test.ll)

