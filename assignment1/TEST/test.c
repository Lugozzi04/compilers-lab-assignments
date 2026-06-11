int test_algebraic(int x) {
    int a = x + 0;
    int b = 0 + a;
    int c = b * 1;
    int d = 1 * c;
    return d;
}

int test_multi(int b) {
    int a = b + 1;
    int c = a - 1;
    return c;
}

int test_strength_mul_left(int x) {
    return x * 15;
}

int test_strength_mul_right(int x) {
    return 15 * x;
}

unsigned test_strength_div8(unsigned x) {
    return x / 8;
}

int main() {
    int a = test_algebraic(10);
    int b = test_multi(20);
    int c = test_strength_mul_left(3);
    int d = test_strength_mul_right(4);
    unsigned e = test_strength_div8(64);

    return a + b + c + d + e;
}