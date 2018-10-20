#include <iostream>
#include <vector>
#include <cassert>
#include <string>

#include "trampoline.h"

using namespace std;

struct test_func {
    int operator()(int c1, int c2, int c3, int c4, int c5) {
        cout << "inside test_func()" << endl;
        assert(c1 == 1 && c2 == 2 && c3 == 3 && c4 == 4 && c5 == 5);
        return 1337;
    }

    test_func() {
        cout << "+ test_func " << (void *) this << endl;
    }

    ~test_func() {
        cout << "- test_func " << (void *) this << endl;
    }
};

void test_func_struct() {
    int near_up = -4;
    test_func func;
    int near_down = -8;

    trampoline<int(int, int, int, int, int)> tramp(func);
    auto foo = tramp.get();
    assert(near_down == -8 && near_up == -4);
    near_down = 1;
    near_up = 0;

    assert(foo(1, 2, 3, 4, 5) == 1337);
    assert(near_down == 1 && near_up == 0);

    cout << "DONE" << endl;
}

void test_lots_of_arguments() {
    trampoline<long long(int, int, int, int, int, int, int, int, int, int)> tramp(
            [&](int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int c10) {
                return c1 - c2 + c3 - c4 + c5 - c6 + c7 - c8 + c9 - c10;
            }
    );
    auto foo = tramp.get();
    {
        assert(foo(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == -5);
    }

    assert(foo(9, 8, 7, 6, 5, 4, 3, 2, 1, 0) == 5);

    cout << "DONE" << endl;
}

void test_lots_of_trampolines() {
    trampoline<long long(int, int, int, int, int, int, int, int, int)> tramp(
            [&](int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9) {
                return c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9;
            }
    );
    trampoline<long long(int, int, int, int, int, int, int, int, int)> tramp1(
            [&](int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9) {
                return c1 - c2 + c3 - c4 + c5 - c6 + c7 - c8 + c9;
            }
    );
    trampoline<double(double, float, long long, int, short, int, int, int, int)> tramp2(
            [&](double c1, float c2, long long c3, int c4, short c5, int c6, int c7, int c8, int c9) {
                return c1 * c2 + c3 * c4 + c5 * c6 + c7 * c8 + c9;
            }
    );
    trampoline<const char *(int, int, int, int, int, int, int, int, int)> tramp3(
            [&](int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9) {
                cout << "inside third" << endl;
                char *text = new char[100];
                sprintf(text, "%d %d %d %d %d %d %d %d %d", c1, c2, c3, c4, c5, c6, c7, c8, c9);
                return text;
            }
    );

    auto foo = tramp.get();
    auto foo1 = tramp1.get();
    auto foo2 = tramp2.get();
    auto foo3 = tramp3.get();

    cout << foo3(1, 2, 3, 4, 5, 6, 7, 8, 9) << endl;
    cout << foo2(1.2, 2.3, 3, 4, 5, 6, 7, 8, 9) << endl;
    assert(abs(foo2(1.2, 2.3, 3, 4, 5, 6, 7, 8, 9) - 109.76) < 1e5);
    assert(foo1(1, 2, 3, 4, 5, 6, 7, 8, 9) == 5);
    assert(foo(1, 2, 3, 4, 5, 6, 7, 8, 9) == 45);
}

void test_loads_of_doubles() {
    trampoline<const char *(double, double, double, float, double, double, double, float, float)> tramp(
            [&](double c1, double c2, double c3, float c4, double c5, double c6, double c7, float c8, float c9) {
                char *text = new char[100];
                sprintf(text, "%.2f %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f", c1, c2, c3, c4, c5, c6, c7, c8,
                        c9);
                return text;
            }
    );

    auto foo = tramp.get();
    auto foo2 = tramp.get();
    cout << foo(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9) << endl;
    cout << foo2(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9) << endl;
}

struct func {
    int x = 10;

    int operator()() {
        return x++;
    }

    explicit func(int x): x(x) {}

    auto get() {
        return [&](){
            return x++;
        };
    }
};

void spam_new_traps() {
    int sum = 0;
    for (int i = 0; i < 1488; i++) {
        auto tramp = new trampoline<int ()>(*(new func(i)));

        int (*foo)() = tramp->get();
        sum += foo();

        auto tramp2 = new trampoline<int ()>(std::move(*tramp));

        int (*foo2)() = tramp2->get();
        sum += foo2();

        assert(tramp->get() == nullptr);
        delete tramp2;
//        cout << (void*) foo2 << ' ';
//        cout << (void**) foo2 << ' ';
//        cout << *(void**) foo2 << endl;

//        auto f = func(i);
//        int (*ptr)() = f;

//        int (*ptr)() = [&]() {
//            return f();
//        };
    }

    assert(sum == 1106328 * 2 + 1488);
}

void small_check() {
    auto f = new func(10);
    trampoline<int ()> tramp(*f);
    delete f;

    int (*foo)() = tramp.get();
    cout << foo();
    cout << foo();
    cout << foo();
    cout << foo();
    auto foo2 = tramp.get();
    cout << foo2();
    cout << foo2();
    cout << foo2();
    cout << foo2();
    cout << endl;

    cout << "Gay bar" << endl;

//    int (*bar)() = f.get();
    auto bar = f->get();
    cout << bar();
    cout << bar();
    cout << bar();
    auto bar2 = f->get();
    cout << bar2();
    cout << bar2();
    cout << endl;

//    int (*pointer)() = f;
}

int main() {
    cout << "1\n";
    test_func_struct();
    cout << "\n2\n";
    test_lots_of_arguments();
    cout << "\n3\n";
    test_lots_of_trampolines();
    cout << "\n4\n";
    test_loads_of_doubles();
    cout << "\n5\n";
    auto start = clock();
    spam_new_traps();
    cout << "TIME : " << (double) (clock() - start) / CLOCKS_PER_SEC << endl;
    cout << "\n7\n";
    small_check();
    return 0;
}
