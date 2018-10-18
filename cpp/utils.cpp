#include <iostream>
#include <ctime>
#include <emmintrin.h>
#include <bitset>

//TODO rewrite
std::string to_string(__m128i var) {
    std::string ret = "|";

    char *val = (char *) &var;
    for(size_t i = 0; i < 16; i++) {
        ret += std::to_string(abs((int)*(val + i)));
        ret += "|";
    }

    return ret;
}

//TODO rewrite
std::string to_string(int var) {
    std::bitset<32> x(var);

    return x.to_string();
}
