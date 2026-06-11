// test.c

void test_basic(int *out, int n, int a, int b) {
    for (int i = 0; i < n; i++) {
        int x = a * b;        // loop-invariant
        int y = x + 10;       // loop-invariant, dipende da x
        out[i] = y + i;       // NON invariant, dipende da i
    }
}

void test_multiple_invariants(int *out, int n, int a, int b, int c) {
    for (int i = 0; i < n; i++) {
        int x = a + b;        // loop-invariant
        int y = x * c;        // loop-invariant, dipende da x
        int z = y - 3;        // loop-invariant, dipende da y

        out[i] = z + i;       // NON invariant
    }
}

void test_not_invariant(int *out, int n, int a) {
    for (int i = 0; i < n; i++) {
        int x = i + a;        // NON invariant, dipende da i
        int y = x * 2;        // NON invariant, dipende da x

        out[i] = y;
    }
}

void test_nested_loops(int *out, int n, int m, int a, int b) {
    for (int i = 0; i < n; i++) {
        int outer = a + b;    // invariant rispetto al loop esterno

        for (int j = 0; j < m; j++) {
            int inner = a * b;       // invariant rispetto al loop interno
            int value = inner + j;   // NON invariant, dipende da j

            out[i * m + j] = outer + value;
        }
    }
}