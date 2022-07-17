
struct A {
    int a;
    int b;
};

void x(struct A *test) { test->a = 2; }

int main() {
    struct A a;
    struct A *b = &a;
    x(b);
}
