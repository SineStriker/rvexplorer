#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [input_file] [output_file]" << endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    // 打开输入文件
    ifstream input(input_file, ios::binary);
    if (!input) {
        cerr << "Error: Could not open input file " << input_file << endl;
        return 1;
    }

    // 打开输出文件
    ofstream output(output_file);
    if (!output) {
        cerr << "Error: Could not open output file " << output_file << endl;
        return 1;
    }

    // 读取输入文件大小
    input.seekg(0, ios::end);
    streampos size = input.tellg();
    input.seekg(0, ios::beg);

    // 分配缓冲区
    char *buffer = new char[size];

    // 读取文件内容到缓冲区
    input.read(buffer, size);

    // 写入头文件内容
    output << "#ifndef BINARY_DATA_H" << endl;
    output << "#define BINARY_DATA_H" << endl;
    output << endl;
    output << "const unsigned char binary_data[] = {";

    // 写入每个字节的十六进制表示
    for (int i = 0; i < size; i++) {
        output << "0x" << setw(2) << setfill('0') << hex << static_cast<int>((buffer[i] + 256) % 256) << ",";
    }

    output << "};" << endl;
    output << endl;
    output << "#endif // BINARY_DATA_H" << endl;

    // 清理资源
    delete[] buffer;
    input.close();
    output.close();

    return 0;
}
