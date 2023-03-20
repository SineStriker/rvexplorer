#ifndef GENERATOR_H
#define GENERATOR_H

#include <cstdio>
#include <iostream>

class Generator {
public:
    Generator(FILE *fp, const std::string &content);
    ~Generator();

    void generate();

private:
    FILE *fp;
    std::string content;
};

#endif // GENERATOR_H
