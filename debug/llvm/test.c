
struct A {
    int a;
    int b;
};

int x(int a, int b) { return a + b; }

int main() {
    int b = 3;
    x(5, b);
}
