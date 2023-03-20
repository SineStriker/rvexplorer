#include <iostream>

#include <cstdio>

#include "generator.h"

#if defined(_WIN32) && ENABLE_WIDE
#    include <Windows.h>
using PathChar = wchar_t;
using PathString = std::wstring;
#else
using PathChar = char;
using PathString = std::string;
#endif

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: expander <input> <output>" << std::endl;
        return 0;
    }

    // Get input and output file
    const PathChar *input_file = nullptr;
    const PathChar *output_file = nullptr;

#if defined(_WIN32) && ENABLE_WIDE
    wchar_t **argvW = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (!argvW) {
        return -1;
    }
    input_file = argvW[1];
    output_file = argvW[2];
    LocalFree(argvW);
#else
    input_file = argv[1];
    output_file = argv[2];
#endif

    // Open file
    FILE *fp = nullptr;
    size_t size = 0;

#if defined(_WIN32) && ENABLE_WIDE
    if (_wfopen_s(&fp, input_file, L"rb") != 0) {
        fp = nullptr;
    }
#else
    fp = fopen(input_file, "rb");
#endif
    if (!fp) {
        std::cerr << "Fail to open input file." << std::endl;
        return -1;
    }

    // Read content
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto buf = new char[size];
    fread(buf, size, 1, fp);
    fclose(fp);

    std::string content(buf, size);
    delete[] buf;

    // Start analyze
#if defined(_WIN32) && ENABLE_WIDE
    if (_wfopen_s(&fp, output_file, L"w") != 0) {
        fp = nullptr;
    }
#else
    fp = fopen(output_file, "w");
#endif
    if (!fp) {
        std::cerr << "Fail to create output file." << std::endl;
        return -1;
    }

    Generator generator(fp, content);
    generator.generate();

    return 0;
}