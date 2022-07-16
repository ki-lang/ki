
struct A {
    int a;
    int b;
};

int xx(int a, int b) { return a + b; }

int main() {
    int (*x)(int a, int b) = xx;
    x(1, 2);
}
