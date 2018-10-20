#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <utility>
#include <sys/mman.h>
#include <string>
#include <iostream>

const int FUNC_SIZE = 128;
const int PAGE_SIZE = 4096;

void **page = nullptr;

void new_page() {
    void *mem = mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(mem != MAP_FAILED);

    page = (void **) mem;
}

void *alloc() {
    if (page == nullptr) {
        new_page();
        assert(page != nullptr);
    }
    void *ans = page;
    page = (void **) *page;
    return ans;
}

void free_addr(void *ptr) {
    *(void **) ptr = page;
    page = (void **) ptr;
}

template<typename ... Args>
struct args;

template<>
struct args<> {
    static const int INT = 0;
    static const int FLOAT = 0;
};

template<typename Any, typename ... Other>
struct args<Any, Other ...> {
    static const int INT = args<Other ...>::INT + 1;
    static const int FLOAT = args<Other ...>::FLOAT;
};

template<typename ... Other>
struct args<float, Other ...> {
    static const int INT = args<Other ...>::INT;
    static const int FLOAT = args<Other ...>::FLOAT + 1;
};

template<typename ... Other>
struct args<double, Other ...> {
    static const int INT = args<Other ...>::INT;
    static const int FLOAT = args<Other ...>::FLOAT + 1;
};

template<typename T>
struct trampoline;

template<typename R, typename... Args>
void swap(trampoline<R(Args...)> &a, trampoline<R(Args...)> &b);

template<typename T, typename ... Args>
struct trampoline<T(Args ...)> {
private:
    static const int REGISTERS = 6;
    static const int BYTE = 8;

    const char *shifts[REGISTERS] = {
            "\x48\x89\xfe", // mov rsi rdi
            "\x48\x89\xf2", // mov rdx rsi
            "\x48\x89\xd1", // mov rcx rdx
            "\x49\x89\xc8", // mov r8 rcx
            "\x4d\x89\xc1", // mov r9 r8
            "\x41\x51"      // push r9
    };

    void *func;
    void (*deleter)(void *);

    void *addr;
    char *code;

    void add(const char *command) {
        for (const char *i = command; *i; i++)
            *(code++) = *i;
    }

    void add(const char *command, int32_t c) {
        add(command);
        *(int32_t *) code = c;
        code += BYTE / 2;
    }

    void add(const char *command, void *c) {
        add(command);
        *(void **) code = c;
        code += BYTE;
    }

    template<typename F>
    static T caller(void *obj, Args ...args) {
        return (*static_cast<F *>(obj))(std::forward<Args>(args)...);
    }

public:
    template<typename F>
    trampoline(F func): func(new F(std::move(func))) {
        deleter = [](void *f) {
            delete static_cast<F *>(f);
        };

        addr = alloc();
        code = (char *) addr;

        if (args<Args ...>::INT >= REGISTERS) {
            std::cout << "using stack" << std::endl;
            std::cout << "float: " << args<Args ...>::FLOAT << std::endl;
            std::cout << "int: " << args<Args ...>::INT << std::endl;

            int stack_size = BYTE * (args<Args ...>::INT - REGISTERS + 1 + std::max(args<Args ...>::FLOAT - BYTE, 0));
            std::cout << "stack size: " << stack_size << std::endl;

            // store ret addr
            add("\x4c\x8b\x1c\x24");                     //move r11 [rsp]
            // shift regs
            for (int i = REGISTERS - 1; i >= 0; i--)
                add(shifts[i]);

            // prep stack
            add("\x48\x89\xe0");                         //mov rax,[rsp]
            add("\x48\x05", stack_size);                 //add rax [stack_size + 8]
            add("\x48\x81\xc4", BYTE);                   //add rsp 8

            char *label_1 = code;

            add("\x48\x39\xe0");                         //cmp rax rsp
            add("\x74");                                 //je

            char *label_2 = code;
            code++;

            add("\x48\x81\xc4\x08");                     // add rsp 8
            code += 3;
            add("\x48\x8b\x3c\x24");                     // mov rdi [rsp]
            add("\x48\x89\x7c\x24\xf8");                 // mov [rsp - 8] rdi
            add("\xeb");                                 // jmp

            *code = label_1 - code - 1;
            code++;
            *label_2 = code - label_2 - 1;

            // shift args
            add("\x4c\x89\x1c\x24");                     //mov [rsp], r11
            add("\x48\x81\xec", stack_size);             //sub rsp, stack_size
            add("\x48\xbf", this->func);                 //mov rdi, func
            add("\x48\xb8", (void *) &caller<F>);        //mov rax, call addr
            add("\xff\xd0");                             //call rax

            // do call
            add("\x41\x59");                             // pop r9
            add("\x4c\x8b\x9c\x24", stack_size - BYTE);  // mov r11 [rsp + stack_size]
            add("\x4c\x89\x1c\x24\xc3");                 // mov [rsp] r11
        } else {

//            std::cout << "using registers" << std::endl;

            for (int i = args<Args ...>::INT - 1; i >= 0; i--)
                add(shifts[i]);
            //mov rdi imm
            add("\x48\xbf", this->func);
            //mov rax imm
            add("\x48\xb8", (void *) &caller<F>);
            //mov jmp raxi
            add("\xff\xe0");
        }
    }

    T (*get() const )(Args ... args) {
        return (T(*)(Args ... args)) addr;
    }

    trampoline(const trampoline &) = delete;

    trampoline(trampoline &&other) {
        func = other.func;
        addr = other.addr;
        deleter = other.deleter;
        other.func = nullptr;
        other.addr = nullptr;
    }

    ~trampoline() {
        if (func) deleter(func);
        free_addr(addr);
    }
};

#endif


#pragma clang diagnostic pop
