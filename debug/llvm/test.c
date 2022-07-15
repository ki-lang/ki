
struct A {
	int a;
	int b;
};

int main() {
	struct A obb;
	struct A *ob = &obb;
	ob->a = 5;
	struct A *x = &obb;
	ob->b = 4;
	return 1;
}
