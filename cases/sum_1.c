static const unsigned char codes[] = {'1', '2', '3', '4', '5'};

int sum() {
    int s = 0;
    for (int i = 0; i < 5; ++i) {
        switch (codes[i]) {
            case '1':
                s += 1;
                break;
            case '2':
                s += 2;
                break;
            case '3':
                s += 3;
                break;
            case '4':
                s += 4;
                break;
            case '5':
                s += 5;
                break;
            default:
                break;
        }
    }
    return s;
}
