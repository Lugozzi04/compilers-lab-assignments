// Test suite for assignment 4.
// The file mixes fusible and non-fusible loop pairs so we can see both the
// happy path and the rejection cases in the generated LLVM IR.

int g_sink;

void fuse_reset_then_bump(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = i;
    }

    for (int i = 0; i < n; ++i) {
        a[i] = a[i] + 1;
    }
}

void fuse_scale_then_shift(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = i * 2;
    }

    for (int i = 0; i < n; ++i) {
        int p = i * 17;
        p = p + 5;
        a[i] = a[i] + 3;
    }
}

void no_fuse_different_trip_count(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = i;
    }

    for (int i = 0; i < n - 1; ++i) {
        a[i] = i + 7;
    }
}

void no_fuse_negative_distance(int *a, int n) {
    for (int i = 0; i + 1 < n; ++i) {
        a[i] = i;
    }

    for (int i = 0; i + 1 < n; ++i) {
        a[i + 1] = i;
    }
}

void no_fuse_gap_between_loops(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = i;
    }

    g_sink += n;

    for (int i = 0; i < n; ++i) {
        a[i] = i + g_sink;
    }
}

void no_fuse_branchy_body(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        if ((i & 1) == 0) {
            a[i] = i;
        } else {
            a[i] = -i;
        }
    }

    for (int i = 0; i < n; ++i) {
        a[i] = a[i] + 5;
    }
}
