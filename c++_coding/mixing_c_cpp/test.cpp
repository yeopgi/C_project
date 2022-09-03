#include <iostream>

extern "C" {
extern void test_c_func();
}

int main(void)
{
    test_c_func();

    printf("i'm a c++ file\n");
    return 0;
}

/**
extern "C" void f(int); // one way
extern "C" {            // another way
int g(double);
double h();
};
void code(int i, double d)
{
    f(i);
    int ii = g(d);
    double dd = h();
    // ...
}
*/
