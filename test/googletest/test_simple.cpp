#include "gtest/gtest.h"

class Calculator {
public:
  int Add(int a, int b) { return a + b; }
  int Sub(int a, int b) { return a - b; }
  int Mul(int a, int b) { return a * b; }
  int Div(int a, int b) {
    if (b == 0)
      return -1;
    return a / b;
  }
};

TEST(CalculatorTest, AddTest) {
  Calculator calc;
  EXPECT_EQ(calc.Add(1, 2), 3);
  EXPECT_EQ(calc.Add(-1, 1), 0);
  EXPECT_EQ(calc.Add(0, 0), 0);

  ASSERT_EQ(calc.Add(10, 20), 30);
}

TEST(CalculatorTest, SubTest) {
    Calculator calc;
    EXPECT_EQ(calc.Sub(5, 3), 2);
    EXPECT_EQ(calc.Sub(3, 5), -2);
    ASSERT_NE(calc.Sub(1, 1), 1);
}

TEST(CalculatorTest, DivTest) {
    Calculator calc;
    EXPECT_EQ(calc.Div(10, 2), 5);
    EXPECT_EQ(calc.Div(5, 0), -1);
    EXPECT_NEAR(1.0/3.0, 0.3333, 0.0001);
}
