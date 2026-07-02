// Clean test suite for assignment 4.
// Positive cases: guarded and unguarded fusion with a real RAW dependence.
// Negative cases: a guarded RAR-only pair and an unguarded pair with no useful
// dependence for fusion.

void guarded_raw(int *__restrict A, int *__restrict B, int n) {
    if (n > 0) {
        for (int i = 0; i < n; ++i)
            A[i] = i;

        for (int i = 0; i < n; ++i)
            B[i] = A[i] + 1;
    }
}

void unguarded_raw(int *__restrict A, int *__restrict B, int n) {
    for (int i = 0; i < n; ++i)
        A[i] = i;

    for (int i = 0; i < n; ++i)
        B[i] = A[i] + 1;
}

int guarded_rar_only(int *__restrict A, int n) {
    int sum = 0;

    if (n > 0) {
        for (int i = 0; i < n; ++i)
            sum += A[i];

        for (int i = 0; i < n; ++i)
            sum += A[i];
    }

    return sum;
}

void unguarded_no_dependence(int *__restrict A, int *__restrict B, int n) {
    for (int i = 0; i < n; ++i)
        A[i] = i;

    for (int i = 0; i < n; ++i)
        B[i] = i + 1;
}

void guarded_negative_dependence(int *__restrict A, int *__restrict B, int n) {
    if (n > 1) {
        for (int i = 0; i < n - 1; ++i)
            A[i] = i;

        for (int i = 0; i < n - 1; ++i)
            B[i] = A[i + 1] + 1;
    }
}
