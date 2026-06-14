// Test suite for assignment 4.
// The loops use small constant trip counts so the simplified fusion pass can
// match the PDF more closely.

#define N 8

int g_sink;

void fuse_reset_then_bump(int *a) {
    for (int i = 0; i < N; ++i) {
        a[i] = i;
    }

    for (int i = 0; i < N; ++i) {
        a[i] = a[i] + 1;
    }
}

void fuse_scale_then_shift(int *a) {
    for (int i = 0; i < N; ++i) {
        a[i] = i * 2;
    }

    for (int i = 0; i < N; ++i) {
        int p = i * 17;
        p = p + 5;
        a[i] = a[i] + 3;
    }
}

void no_fuse_different_trip_count(int *a) {
    for (int i = 0; i < N; ++i) {
        a[i] = i;
    }

    for (int i = 0; i < N - 1; ++i) {
        a[i] = i + 7;
    }
}

void no_fuse_negative_distance(int *src, int *dst) {
    for (int i = 0; i < N - 3; ++i) {
        src[i] = i;
    }

    for (int i = 0; i < N - 3; ++i) {
        dst[i] = src[i + 3];
    }
}

void no_fuse_gap_between_loops(int *a) {
    for (int i = 0; i < N; ++i) {
        a[i] = i;
    }

    g_sink += N;

    for (int i = 0; i < N; ++i) {
        a[i] = i + g_sink;
    }
}

void fuse_branchy_body(int *a) {
    for (int i = 0; i < N; ++i) {
        if ((i & 1) == 0) {
            a[i] = i;
        } else {
            a[i] = -i;
        }
    }

    for (int i = 0; i < N; ++i) {
        a[i] = a[i] + 5;
    }
}
