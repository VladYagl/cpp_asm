#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>
#include <algorithm>
#include <emmintrin.h>

#include "utils.cpp"

bool verbose = false;

void check(size_t (*word_count)(std::string&), std::string& test_str, size_t correct_ans) {
    auto start = std::clock();
    size_t ans = word_count(test_str);
    std::cout << (double)(std::clock() - start) / CLOCKS_PER_SEC << std::endl;
    if (ans != correct_ans) {
        std::cout << "They are different ¯\\_(ツ)_/¯ " << ans << ' ' << correct_ans << std::endl;
        exit(0);
    }
}

size_t dummy_count(std::string& str) {
    bool new_word = 1;
    size_t ans = 0;
    for (auto i : str) {
        if (i == ' ') {
            new_word = 1;
        } else {
            ans += new_word;
            new_word = false;
        }
    }

    return ans;
}

size_t asm_count(std::string& input) {
    const char* str = input.c_str();
    size_t size = input.size();
    size_t pos = 0;
    size_t ans = 0;

    __m128i space_mask = _mm_set_epi8(' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    __m128i curr, next = __m128i();

    __asm__ volatile(
            "movdqa     (%2), %0\n"    // next = pos
            "pcmpeqb    %1,   %0\n"    // next = next == space_mask
            : "=ax" (next)
            : "bx" (space_mask), "r" (str + pos)
            : "memory", "cc"
            );

    for (; pos + 16 < size; pos += 16) {
        int32_t a;
        __asm__ volatile(
                "movdqa     %1,     %0\n"            // curr = next
                "movdqa     (%4),   %%xmm7\n"        // tmp = pos

                "pcmpeqb    %2,     %%xmm7\n"        // tmp = tmp == space_mask
                "movdqa     %%xmm7, %1\n"            // next = tmp

                "palignr    $1,     %0,    %%xmm7\n" // tmp = (tmp . curr) << 1
                "pandn      %0,     %%xmm7\n"        // tmp = !tmp & curr
                "pmovmskb   %%xmm7, %3\n"            // a = pack tmp
                : "=x" (curr), "=x" (next), "=x" (space_mask), "=r" (a)
                : "r" (str + pos + 16), "0" (curr), "1" (next), "2" (space_mask)
                : "memory", "cc"
                );

        if (verbose) {
            std::cout << "next: " << to_string(next) << std::endl;
            std::cout << "curr: " << to_string(curr) << std::endl;

            std::cout << "a   : " << to_string(a) << std::endl;
        }

        ans += __builtin_popcount(a);
    }

    bool new_word = (str[0] == ' ');
    for (;pos < size; pos++) {
        if (str[pos] == ' ') {
            new_word = 1;
        } else {
            ans += new_word;
            new_word = 0;
        }
    }

    return ans + (str[0] != ' ');
}

int main() {

    /* size_t seed = time(0); */
    size_t seed = 1539869177;
    while (true) {
        std::cout << seed << std::endl;
        srand(seed++);

        size_t correct_ans = 0;
        bool new_word = true;
        size_t N = 32;
        verbose = true;
        char* str = new char[N];

        for (size_t i = 0; i < N; i++) {
            str[i] = (char)(rand() % 20 + 'a');
            correct_ans += new_word;
            new_word = false;
            if(rand() % 13 == 0) {
                size_t space = rand() % 15 + 1;
                for (size_t j = 1; j <= space && i + j < N; j++) {
                    str[i + j] = ' ';
                }
                i += space;
                new_word = 1;
            }
        }

        std::string test_str = std::string(str);
        if (verbose) {
            std::cout << test_str << "|$$$| " << correct_ans << std::endl;
        }

        std::cout << "dummy count" << std::endl;
        check(dummy_count, test_str, correct_ans);

        std::cout << "asm count" << std::endl;
        check(asm_count, test_str, correct_ans);
        
        std::cout << std::endl;
    }

    return 0;
}
