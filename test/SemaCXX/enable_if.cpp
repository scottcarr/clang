// RUN: %clang_cc1 -std=c++11 -verify %s

typedef int (*fp)(int);
int surrogate(int);

struct X {
  X() = default;  // expected-note{{candidate constructor not viable: requires 0 arguments, but 1 was provided}}
  X(const X&) = default;  // expected-note{{candidate constructor not viable: no known conversion from 'bool' to 'const X' for 1st argument}}
  X(bool b) __attribute__((enable_if(b, "chosen when 'b' is true")));  // expected-note{{candidate disabled: chosen when 'b' is true}}

  void f(int n) __attribute__((enable_if(n == 0, "chosen when 'n' is zero")));
  void f(int n) __attribute__((enable_if(n == 1, "chosen when 'n' is one")));  // expected-note{{member declaration nearly matches}} expected-note{{candidate disabled: chosen when 'n' is one}}

  void g(int n) __attribute__((enable_if(n == 0, "chosen when 'n' is zero")));  // expected-note{{candidate disabled: chosen when 'n' is zero}}

  void h(int n, int m = 0) __attribute((enable_if(m == 0, "chosen when 'm' is zero")));  // expected-note{{candidate disabled: chosen when 'm' is zero}}

  static void s(int n) __attribute__((enable_if(n == 0, "chosen when 'n' is zero")));  // expected-note2{{candidate disabled: chosen when 'n' is zero}}

  void conflict(int n) __attribute__((enable_if(n+n == 10, "chosen when 'n' is five")));  // expected-note{{candidate function}}
  void conflict(int n) __attribute__((enable_if(n*2 == 10, "chosen when 'n' is five")));  // expected-note{{candidate function}}

  operator long() __attribute__((enable_if(true, "chosen on your platform")));
  operator int() __attribute__((enable_if(false, "chosen on other platform")));

  operator fp() __attribute__((enable_if(false, "never enabled"))) { return surrogate; }  // expected-note{{conversion candidate of type 'int (*)(int)'}}  // FIXME: the message is not displayed
};

void X::f(int n) __attribute__((enable_if(n == 0, "chosen when 'n' is zero")))  // expected-note{{member declaration nearly matches}} expected-note{{candidate disabled: chosen when 'n' is zero}}
{
}

void X::f(int n) __attribute__((enable_if(n == 2, "chosen when 'n' is two")))  // expected-error{{out-of-line definition of 'f' does not match any declaration in 'X'}} expected-note{{candidate disabled: chosen when 'n' is two}}
{
}

X x1(true);
X x2(false); // expected-error{{no matching constructor for initialization of 'X'}}

__attribute__((deprecated)) constexpr int old() { return 0; }  // expected-note2{{'old' has been explicitly marked deprecated here}}
void deprec1(int i) __attribute__((enable_if(old() == 0, "chosen when old() is zero")));  // expected-warning{{'old' is deprecated}}
void deprec2(int i) __attribute__((enable_if(old() == 0, "chosen when old() is zero")));  // expected-warning{{'old' is deprecated}}

void overloaded(int);
void overloaded(long);

struct Int {
  constexpr Int(int i) : i(i) { }
  constexpr operator int() const { return i; }
  int i;
};

void default_argument(int n, int m = 0) __attribute__((enable_if(m == 0, "chosen when 'm' is zero")));  // expected-note{{candidate disabled: chosen when 'm' is zero}}
void default_argument_promotion(int n, int m = Int(0)) __attribute__((enable_if(m == 0, "chosen when 'm' is zero")));  // expected-note{{candidate disabled: chosen when 'm' is zero}}

struct Nothing { };
template<typename T> void typedep(T t) __attribute__((enable_if(t, "")));  // expected-note{{candidate disabled:}}  expected-error{{value of type 'Nothing' is not contextually convertible to 'bool'}}
template<int N> void valuedep() __attribute__((enable_if(N == 1, "")));

// FIXME: we skip potential constant expression evaluation on value dependent
// enable-if expressions
int not_constexpr();
template<int N> void valuedep() __attribute__((enable_if(N == not_constexpr(), "")));

template <typename T> void instantiationdep() __attribute__((enable_if(sizeof(sizeof(T)) != 0, "")));

void test() {
  X x;
  x.f(0);
  x.f(1);
  x.f(2);  // no error, suppressed by erroneous out-of-line definition
  x.f(3);  // expected-error{{no matching member function for call to 'f'}}

  x.g(0);
  x.g(1);  // expected-error{{no matching member function for call to 'g'}}

  x.h(0);
  x.h(1, 2);  // expected-error{{no matching member function for call to 'h'}}

  x.s(0);
  x.s(1);  // expected-error{{no matching member function for call to 's'}}

  X::s(0);
  X::s(1);  // expected-error{{no matching member function for call to 's'}}

  x.conflict(5);  // expected-error{{call to member function 'conflict' is ambiguous}}

  deprec2(0);

  overloaded(x);

  default_argument(0);
  default_argument(1, 2);  // expected-error{{no matching function for call to 'default_argument'}}

  default_argument_promotion(0);
  default_argument_promotion(1, 2);  // expected-error{{no matching function for call to 'default_argument_promotion'}}

  int i = x(1);  // expected-error{{no matching function for call to object of type 'X'}}

  Nothing n;
  typedep(0);  // expected-error{{no matching function for call to 'typedep'}}
  typedep(1);
  typedep(n);  // expected-note{{in instantiation of function template specialization 'typedep<Nothing>' requested here}}
}

template <typename T> class C {
  void f() __attribute__((enable_if(T::expr == 0, ""))) {}
  void g() { f(); }
};

int fn3(bool b) __attribute__((enable_if(b, "")));
template <class T> void test3() {
  fn3(sizeof(T) == 1);
}

template <typename T>
struct Y {
  T h(int n, int m = 0) __attribute__((enable_if(m == 0, "chosen when 'm' is zero")));  // expected-note{{candidate disabled: chosen when 'm' is zero}}
};

void test4() {
  Y<int> y;

  int t0 = y.h(0);
  int t1 = y.h(1, 2);  // expected-error{{no matching member function for call to 'h'}}
}

// FIXME: issue an error (without instantiation) because ::h(T()) is not
// convertible to bool, because return types aren't overloadable.
void h(int);
template <typename T> void outer() {
  void local_function() __attribute__((enable_if(::h(T()), "")));
  local_function();
};

namespace PR20988 {
  struct Integer {
    Integer(int);
  };

  int fn1(const Integer &) __attribute__((enable_if(true, "")));
  template <class T> void test1() {
    int &expr = T::expr();
    fn1(expr);
  }

  int fn2(const Integer &) __attribute__((enable_if(false, "")));  // expected-note{{candidate disabled}}
  template <class T> void test2() {
    int &expr = T::expr();
    fn2(expr);  // expected-error{{no matching function for call to 'fn2'}}
  }

  int fn3(bool b) __attribute__((enable_if(b, "")));
  template <class T> void test3() {
    fn3(sizeof(T) == 1);
  }
}
