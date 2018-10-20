#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>
#include <algorithm>
#include <emmintrin.h>

#include "utils.cpp"

bool verbose = false;

void check(size_t (*word_count)(const char*, size_t), const char* str, size_t size, size_t correct_ans) {
    auto start = std::clock();
    size_t ans = word_count(str, size);
    std::cout << (double) (std::clock() - start) / CLOCKS_PER_SEC << std::endl;
    if (ans != correct_ans) {
        std::cout << "They are different ¯\\_(ツ)_/¯ " << ans << ' ' << correct_ans << std::endl;
        exit(0);
    }
}

size_t dummy_count(const char *str, size_t size) {
    bool new_word = true;
    size_t ans = 0;
    for (int i = 0; i < size; i++) {
        if (str[i] == ' ') {
            new_word = true;
        } else {
            ans += new_word;
            new_word = false;
        }
    }

    return ans;
}

size_t asm_count(const char *str, size_t size) {
    size_t pos = 0;
    size_t ans = 0;

    bool new_word = true;
    for (; (reinterpret_cast<size_t>(str) + pos) % 16 != 0 && pos < size; pos++) {
        if (str[pos] == ' ') {
            new_word = true;
        } else {
            ans += new_word;
            new_word = false;
        }
    }

    if (pos == size) return ans;

    __m128i space_mask = _mm_set_epi8(' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    __m128i curr, next;

    size_t first = pos;

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

//        if (verbose) {
//            std::cout << "next: " << to_string(next) << std::endl;
//            std::cout << "curr: " << to_string(curr) << std::endl;
//
//            std::cout << "a   : " << to_string(a) << std::endl;
//        }

        ans += __builtin_popcount(a);
    }

    new_word = false;
    for (; pos < size; pos++) {
        if (str[pos] == ' ') {
            new_word = true;
        } else {
            ans += new_word;
            new_word = false;
        }
    }

    return ans + (str[first] != ' ' && (first == 0 || str[first - 1] == ' '));
}

int main() {

//    std::cout << asm_count(" 888888 888888888888888888888888") << std::endl;
//    std::cout << dummy_count(" 888888 888888888888888888888888") << std::endl;
//
//    return 0;

    time_t seed = time(nullptr);
    /* size_t seed = 1539869177; */
    while (true) {
        std::cout << seed << std::endl;
        srand(seed++);

        size_t correct_ans = 0;
        size_t N = 128;
        size_t shift = rand() % 16;
//        verbose = true;
        char *str = new char[N + shift];
        str += shift;

        for (size_t i = 0; i < N; i++) {
            if (rand() % 20 > 10) {
                str[i] = (char) (rand() % 20 + 'a');
            } else {
                str[i] = ' ';
            }
        }

        std::string test_str = std::string(str);
        correct_ans = dummy_count(str, N);
        if (verbose) {
            std::cout << test_str << "|$$$| " << correct_ans << std::endl;
        }

        std::cout << "dummy count" << std::endl;
        check(dummy_count, str, N, correct_ans);

        std::cout << "asm count" << std::endl;
        check(asm_count, str, N, correct_ans);

        std::cout << std::endl;
    }

    return 0;
}