#include <gtest/gtest.h>

#include <iostream>

#include "educelab/core/types/Signals.hpp"

using namespace educelab;

static int FREE_INT_VAL{0};
static void FreeFnIntSlot(int i) { FREE_INT_VAL = i; }

static float FREE_FLOAT_VAL{0};
static void FreeFnFloatSlot(float f) { FREE_FLOAT_VAL = f; }

static void FreeFnIntFloatSlot(int i, float f)
{
    FREE_INT_VAL = i;
    FREE_FLOAT_VAL = f;
}

static void FreeFnIntRefSlot(int& i) { i = 1; }
static void FreeFnIntPtrSlot(int* i) { FreeFnIntRefSlot(*i); }

[[noreturn]] static void FreeFnExit()
{
    std::cerr << "Success" << std::endl;
    std::exit(0);
}

struct Receiver {
    int intVal{0};
    float floatVal{0};

    void intSlot(int i) { intVal = i; }
    void floatSlot(float f) { floatVal = f; }
    [[noreturn]] void exitSlot()
    {
        std::cerr << "Success" << std::endl;
        std::exit(intVal);
    }

    static void StaticFreeIntSlot(int i) { FREE_INT_VAL = i; }
    static void StaticFreeFloatSlot(float f) { FREE_FLOAT_VAL = f; }
};

TEST(Signals, FreeFnConnection)
{
    Signal<float> signal;
    signal.connect(FreeFnIntSlot);
    signal.connect(&FreeFnFloatSlot);

    // Test send
    signal.send(1.5);
    EXPECT_EQ(FREE_INT_VAL, 1);
    EXPECT_EQ(FREE_FLOAT_VAL, 1.5);

    // Test () operator
    signal(2.5);
    EXPECT_EQ(FREE_INT_VAL, 2);
    EXPECT_EQ(FREE_FLOAT_VAL, 2.5);
}

TEST(Signals, MemberFnConnection)
{
    Signal<float> signal;
    Receiver r;
    signal.connect(&r, &Receiver::intSlot);
    signal.connect(&r, &Receiver::floatSlot);

    signal.send(1.5);
    EXPECT_EQ(r.intVal, 1);
    EXPECT_EQ(r.floatVal, 1.5);
}

TEST(Signals, LambdaFnConnection)
{
    float floatVal{0};
    int intVal{0};

    Signal<float> signal;
    signal.connect([&intVal](int v) { intVal = v; });
    signal.connect([&floatVal](float v) { floatVal = v; });

    signal.send(1.5);
    EXPECT_EQ(intVal, 1);
    EXPECT_EQ(floatVal, 1.5);
}

TEST(Signals, StaticMemberFn)
{
    Signal<float> signal;

    signal.connect(Receiver::StaticFreeIntSlot);
    signal.connect(Receiver::StaticFreeFloatSlot);

    signal.send(1.5);
    EXPECT_EQ(FREE_INT_VAL, 1);
    EXPECT_EQ(FREE_FLOAT_VAL, 1.5);
}

TEST(Signals, MultiParameterFn)
{
    Signal<int, float> signal;
    signal.connect(FreeFnIntFloatSlot);
    signal.send(1, 1.5);
    EXPECT_EQ(FREE_INT_VAL, 1);
    EXPECT_EQ(FREE_FLOAT_VAL, 1.5);

    Signal<float, int> swapped;
    swapped.connect(FreeFnIntFloatSlot);
    swapped.send(2.5, 2);
    EXPECT_EQ(FREE_INT_VAL, 2);
    EXPECT_EQ(FREE_FLOAT_VAL, 2);
}

TEST(Signals, NoParameterFreeFn)
{
    Signal<> signal;
    signal.connect(FreeFnExit);
    using ::testing::ExitedWithCode;
    EXPECT_EXIT(signal(), ExitedWithCode(0), "Success");
}

TEST(Signals, NoParameterMemberFn)
{
    Signal<> signal;
    Receiver r;
    signal.connect(&r, &Receiver::exitSlot);
    using ::testing::ExitedWithCode;
    EXPECT_EXIT(signal(), ExitedWithCode(0), "Success");
}

TEST(Signals, NoParameterLambdaFn)
{
    Signal<> signal;
    signal.connect([]() { FreeFnExit(); });
    using ::testing::ExitedWithCode;
    EXPECT_EXIT(signal(), ExitedWithCode(0), "Success");
}

TEST(Signals, NoParameterSlot)
{
    Signal<int> signal;
    signal.connect(FreeFnExit);
    using ::testing::ExitedWithCode;
    EXPECT_EXIT(signal.send(1), ExitedWithCode(0), "Success");
}

TEST(Signals, NoParameterMemberSlot)
{
    Signal<int> signal;
    Receiver r;
    signal.connect(&r, &Receiver::exitSlot);
    using ::testing::ExitedWithCode;
    EXPECT_EXIT(signal(0), ExitedWithCode(0), "Success");
}

TEST(Signals, ReferenceParameterFn)
{
    Signal<int&> signal;
    signal.connect(FreeFnIntRefSlot);
    int val{0};
    signal.send(val);
    EXPECT_EQ(val, 1);
}

TEST(Signals, PointerParameterFn)
{
    Signal<int*> signal;
    signal.connect(FreeFnIntPtrSlot);

    auto val = std::make_unique<int>();
    signal.send(val.get());
    EXPECT_EQ(*val, 1);
}

TEST(Signals, Sendlval)
{
    size_t val{0};
    Signal<size_t> signal;
    signal.connect([&val](size_t v) { val = v; });

    size_t i{1};
    signal(i);

    EXPECT_EQ(val, 1);
}