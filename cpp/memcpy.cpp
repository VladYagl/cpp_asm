#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>
#include <algorithm>
#include <emmintrin.h>

const size_t N = (1 << 30) - 31;

std::vector<char> src(N);

inline void bytecpy(void *dest, void const *src, const size_t pos) {
    *(static_cast<char *>(dest) + pos) = *(static_cast<char const *>(src) + pos);
}

void *memcpy_dummy(void *dest, void const *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        *(static_cast<char *>(dest) + i) = *(static_cast<char const *>(src) + i);
    }
    return dest;
}

void *memcpy_asm(void *dest, const void *src, size_t count) {
    size_t pos = 0;
    for (; (reinterpret_cast<size_t>(dest) + pos) % 16 != 0 && pos < count; pos++) {
        bytecpy(dest, src, pos);
    }

    for (; pos + 16 < count; pos += 16) {
        __m128i tmp;
        __asm__ volatile (
        "movdqu (%1, %3), %0;"
        "movntdq %0, (%2, %3);"
        : "=x"(tmp)
        : "r"(dest), "r"(src), "r"(pos)
        : "memory"
        );
    }

    for (; pos < count; pos++) {
        bytecpy(dest, src, pos);
    }

    _mm_sfence();
    return dest;
}

void check(void *(*memcpy)(void *, void const *, size_t)) {
    size_t shift = 7;
    auto dest = std::vector<char>(N);

    auto start = std::clock();
    memcpy(dest.data() + shift, src.data() + shift, N - shift);
    std::cout << (double) (std::clock() - start) / CLOCKS_PER_SEC << std::endl;
    memcpy(dest.data(), src.data(), shift);
    if (dest != src) {
        std::cout << "They are different ¯\\_(ツ)_/¯" << std::endl;
    }
}

int main() {
    while (true) {
        size_t x = rand();
        for (size_t i = 0; i < N; i++) {
            src[i] = static_cast<char>((i + 1) * x);
        }

        std::cout << "asm memcpy" << std::endl;
        check(memcpy_asm);
        std::cout << "default memcpy" << std::endl;
        check(memcpy);
        std::cout << "dummy memcpy" << std::endl;
        check(memcpy_dummy);
    }

    return 0;
}
