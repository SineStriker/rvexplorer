static const unsigned char codes[] = {'1', '2', '3', '4', '5'};

int sum() {
    int s = 0;
    for (int i = 0; i < 5; ++i) {
        s += codes[i];
    }
    return s;
}
