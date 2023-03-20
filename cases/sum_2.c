static const unsigned char codes[] = {'1', '2', '3', '4', '5'};

inline int isPrime(int n) {
    for (int i = 2; i < n; ++i) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

int sum() {
    int s = 0;
    for (int i = 0; i < 5; ++i) {
        if (isPrime(codes[i])) {
            s += codes[i];
        }
    }
    return s;
}