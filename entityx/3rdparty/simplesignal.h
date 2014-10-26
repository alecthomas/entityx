// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef SIMPLE_SIGNAL_H__
#define SIMPLE_SIGNAL_H__

#include <assert.h>
#include <stdint.h>
#include <vector>
#include <functional>

namespace Simple {

namespace Lib {

/// ProtoSignal is the template implementation for callback list.
template <typename, typename>
class ProtoSignal;  // undefined

/// CollectorInvocation invokes signal handlers differently depending on return
/// type.
template <typename, typename>
struct CollectorInvocation;

/// CollectorLast returns the result of the last signal handler from a signal
/// emission.
template <typename Result>
struct CollectorLast {
  typedef Result CollectorResult;
  explicit CollectorLast() : last_() {}
  inline bool operator()(Result r) {
    last_ = r;
    return true;
  }
  CollectorResult result() { return last_; }

 private:
  Result last_;
};

/// CollectorDefault implements the default signal handler collection behaviour.
template <typename Result>
struct CollectorDefault : CollectorLast<Result> {};

/// CollectorDefault specialisation for signals with void return type.
template <>
struct CollectorDefault<void> {
  typedef void CollectorResult;
  void result() {}
  inline bool operator()(void) { return true; }
};

/// CollectorInvocation specialisation for regular signals.
template <class Collector, class R, class... Args>
struct CollectorInvocation<Collector, R(Args...)> {
  inline bool invoke(Collector &collector, const std::function<R(Args...)> &cbf,
                     Args... args) {
    return collector(cbf(args...));
  }
};

/// CollectorInvocation specialisation for signals with void return type.
template <class Collector, class... Args>
struct CollectorInvocation<Collector, void(Args...)> {
  inline bool invoke(Collector &collector,
                     const std::function<void(Args...)> &cbf, Args... args) {
    cbf(args...);
    return collector();
  }
};

/// ProtoSignal template specialised for the callback signature and collector.
template <class Collector, class R, class... Args>
class ProtoSignal<R(Args...), Collector> : private CollectorInvocation<
                                               Collector, R(Args...)> {
 protected:
  typedef std::function<R(Args...)> CbFunction;
  typedef typename CbFunction::result_type Result;
  typedef typename Collector::CollectorResult CollectorResult;

 private:
  /// SignalLink implements a doubly-linked ring with ref-counted nodes
  /// containing the signal handlers.
  struct SignalLink {
    SignalLink *next, *prev;
    CbFunction function;
    int ref_count;
    explicit SignalLink(const CbFunction &cbf)
        : next(0), prev(0), function(cbf), ref_count(1) {}
    /*dtor*/ ~SignalLink() { assert(ref_count == 0); }
    void incref() {
      ref_count += 1;
      assert(ref_count > 0);
    }
    void decref() {
      ref_count -= 1;
      if (!ref_count)
        delete this;
      else
        assert(ref_count > 0);
    }
    void unlink() {
      function = nullptr;
      if (next) next->prev = prev;
      if (prev) prev->next = next;
      decref();
      // leave intact ->next, ->prev for stale iterators
    }
    size_t add_before(const CbFunction &cb) {
      SignalLink *link = new SignalLink(cb);
      link->prev = prev;  // link to last
      link->next = this;
      prev->next = link;  // link from last
      prev = link;
      static_assert(sizeof(link) == sizeof(size_t), "sizeof size_t");
      return size_t(link);
    }
    bool deactivate(const CbFunction &cbf) {
      if (cbf == function) {
        function = 0;  // deactivate static head
        return true;
      }
      for (SignalLink *link = this->next ? this->next : this; link != this;
           link = link->next)
        if (cbf == link->function) {
          link->unlink();  // deactivate and unlink sibling
          return true;
        }
      return false;
    }
    bool remove_sibling(size_t id) {
      for (SignalLink *link = this->next ? this->next : this; link != this;
           link = link->next)
        if (id == size_t(link)) {
          link->unlink();  // deactivate and unlink sibling
          return true;
        }
      return false;
    }
  };
  SignalLink *callback_ring_;  // linked ring of callback nodes
  /*copy-ctor*/ ProtoSignal(const ProtoSignal &) = delete;
  ProtoSignal &operator=(const ProtoSignal &) = delete;
  void ensure_ring() {
    if (!callback_ring_) {
      callback_ring_ = new SignalLink(CbFunction());  // ref_count = 1
      callback_ring_->incref();  // ref_count = 2, head of ring, can be
                                 // deactivated but not removed
      callback_ring_->next = callback_ring_;  // ring head initialization
      callback_ring_->prev = callback_ring_;  // ring tail initialization
    }
  }

 public:
  /// ProtoSignal constructor, connects default callback if non-0.
  ProtoSignal(const CbFunction &method) : callback_ring_(0) {
    if (method != 0) {
      ensure_ring();
      callback_ring_->function = method;
    }
  }
  /// ProtoSignal destructor releases all resources associated with this signal.
  ~ProtoSignal() {
    if (callback_ring_) {
      while (callback_ring_->next != callback_ring_)
        callback_ring_->next->unlink();
      assert(callback_ring_->ref_count >= 2);
      callback_ring_->decref();
      callback_ring_->decref();
    }
  }
  /// Operator to add a new function or lambda as signal handler, returns a
  /// handler connection ID.
  size_t connect(const CbFunction &cb) {
    ensure_ring();
    return callback_ring_->add_before(cb);
  }
  /// Operator to remove a signal handler through it connection ID, returns if a
  /// handler was removed.
  bool disconnect(size_t connection) {
    return callback_ring_ ? callback_ring_->remove_sibling(connection) : false;
  }
  /// Emit a signal, i.e. invoke all its callbacks and collect return types with
  /// the Collector.
  CollectorResult emit(Args... args) {
    Collector collector;
    if (!callback_ring_) return collector.result();
    SignalLink *link = callback_ring_;
    link->incref();
    do {
      if (link->function != 0) {
        const bool continue_emission =
            this->invoke(collector, link->function, args...);
        if (!continue_emission) break;
      }
      SignalLink *old = link;
      link = old->next;
      link->incref();
      old->decref();
    } while (link != callback_ring_);
    link->decref();
    return collector.result();
  }
  // Number of connected slots.
  std::size_t size() {
    std::size_t size = 0;
    SignalLink *link = callback_ring_;
    link->incref();
    do {
      if (link->function != 0) {
        size++;
      }
      SignalLink *old = link;
      link = old->next;
      link->incref();
      old->decref();
    } while (link != callback_ring_);
    return size;
  }
};

}  // Lib
   // namespace Simple

/**
 * Signal is a template type providing an interface for arbitrary callback
 * lists.
 * A signal type needs to be declared with the function signature of its
 * callbacks,
 * and optionally a return result collector class type.
 * Signal callbacks can be added with operator+= to a signal and removed with
 * operator-=, using
 * a callback connection ID return by operator+= as argument.
 * The callbacks of a signal are invoked with the emit() method and arguments
 * according to the signature.
 * The result returned by emit() depends on the signal collector class. By
 * default, the result of
 * the last callback is returned from emit(). Collectors can be implemented to
 * accumulate callback
 * results or to halt a running emissions in correspondance to callback results.
 * The signal implementation is safe against recursion, so callbacks may be
 * removed and
 * added during a signal emission and recursive emit() calls are also safe.
 * The overhead of an unused signal is intentionally kept very low, around the
 * size of a single pointer.
 * Note that the Signal template types is non-copyable.
 */
template <typename SignalSignature,
          class Collector = Lib::CollectorDefault<
              typename std::function<SignalSignature>::result_type>>
struct Signal /*final*/ : Lib::ProtoSignal<SignalSignature, Collector> {
  typedef Lib::ProtoSignal<SignalSignature, Collector> ProtoSignal;
  typedef typename ProtoSignal::CbFunction CbFunction;
  /// Signal constructor, supports a default callback as argument.
  Signal(const CbFunction &method = CbFunction()) : ProtoSignal(method) {}
};

/// This function creates a std::function by binding @a object to the member
/// function pointer @a method.
template <class Instance, class Class, class R, class... Args>
std::function<R(Args...)> slot(Instance &object, R (Class::*method)(Args...)) {
  return [&object, method](Args... args) { return (object.*method)(args...); };
}

/// This function creates a std::function by binding @a object to the member
/// function pointer @a method.
template <class Class, class R, class... Args>
std::function<R(Args...)> slot(Class *object, R (Class::*method)(Args...)) {
  return [object, method](Args... args) { return (object->*method)(args...); };
}

/// Keep signal emissions going while all handlers return !0 (true).
template <typename Result>
struct CollectorUntil0 {
  typedef Result CollectorResult;
  explicit CollectorUntil0() : result_() {}
  const CollectorResult &result() { return result_; }
  inline bool operator()(Result r) {
    result_ = r;
    return result_ ? true : false;
  }

 private:
  CollectorResult result_;
};

/// Keep signal emissions going while all handlers return 0 (false).
template <typename Result>
struct CollectorWhile0 {
  typedef Result CollectorResult;
  explicit CollectorWhile0() : result_() {}
  const CollectorResult &result() { return result_; }
  inline bool operator()(Result r) {
    result_ = r;
    return result_ ? false : true;
  }

 private:
  CollectorResult result_;
};

/// CollectorVector returns the result of the all signal handlers from a signal
/// emission in a std::vector.
template <typename Result>
struct CollectorVector {
  typedef std::vector<Result> CollectorResult;
  const CollectorResult &result() { return result_; }
  inline bool operator()(Result r) {
    result_.push_back(r);
    return true;
  }

 private:
  CollectorResult result_;
};

}  // Simple

#endif  // SIMPLE_SIGNAL_H__

#ifdef ENABLE_SIMPLE_SIGNAL_TESTS

#include <string>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

static std::string string_printf(const char *format, ...)
    __attribute__((__format__(__printf__, 1, 2)));
static std::string string_printf(const char *format, ...) {
  std::string result;
  char *str = 0;
  va_list args;
  va_start(args, format);
  if (vasprintf(&str, format, args) >= 0) result = str;
  va_end(args);
  if (str) free(str);
  return result;
}

static uint64_t timestamp_benchmark() {
  struct timespec tp = {0, 0};

#ifdef __MACH__  // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  tp.tv_sec = mts.tv_sec;
  tp.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, &tp);
#endif
  uint64_t stamp = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
  return stamp;
}

struct TestCounter {
  static uint64_t get();
  static void set(uint64_t);
  static void add2(void *, uint64_t);
};

namespace {  // Anon
void (*test_counter_add2)(void *, uint64_t) =
    TestCounter::add2;  // external symbol to prevent easy inlining
static uint64_t test_counter_var = 0;
}  // Anon

class BasicSignalTests {
  static std::string accu;
  struct Foo {
    char foo_bool(float f, int i, std::string s) {
      accu += string_printf("Foo: %.2f\n", f + i + s.size());
      return true;
    }
  };
  static char float_callback(float f, int, std::string) {
    accu += string_printf("float: %.2f\n", f);
    return 0;
  }

 public:
  static void run() {
    accu = "";
    Simple::Signal<char(float, int, std::string)> sig1;
    size_t id1 = sig1.connect(float_callback);
    size_t id2 = sig1.connect([](float, int i, std::string) {
      accu += string_printf("int: %d\n", i);
      return 0;
    });
    size_t id3 = sig1.connect([](float, int, const std::string &s) {
      accu += string_printf("string: %s\n", s.c_str());
      return 0;
    });
    sig1.emit(.3, 4, "huhu");
    bool success;
    success = sig1.disconnect(id1);
    assert(success == true);
    success = sig1.disconnect(id1);
    assert(success == false);
    success = sig1.disconnect(id2);
    assert(success == true);
    success = sig1.disconnect(id3);
    assert(success == true);
    success = sig1.disconnect(id3);
    assert(success == false);
    success = sig1.disconnect(id2);
    assert(success == false);
    Foo foo;
    sig1.connect(Simple::slot(foo, &Foo::foo_bool));
    sig1.connect(Simple::slot(&foo, &Foo::foo_bool));
    sig1.emit(.5, 1, "12");

    Simple::Signal<void(std::string, int)> sig2;
    sig2.connect([](std::string msg,
                    int) { accu += string_printf("msg: %s", msg.c_str()); });
    sig2.connect([](std::string,
                    int d) { accu += string_printf(" *%d*\n", d); });
    sig2.emit("in sig2", 17);

    accu += "DONE";

    const char *expected =
        "float: 0.30\n"
        "int: 4\n"
        "string: huhu\n"
        "Foo: 3.50\n"
        "Foo: 3.50\n"
        "msg: in sig2 *17*\n"
        "DONE";
    assert(accu == expected);
  }
};
std::string BasicSignalTests::accu;

class TestCollectorVector {
  static int handler1() { return 1; }
  static int handler42() { return 42; }
  static int handler777() { return 777; }

 public:
  static void run() {
    Simple::Signal<int(), Simple::CollectorVector<int>> sig_vector;
    sig_vector.connect(handler777);
    sig_vector.connect(handler42);
    sig_vector.connect(handler1);
    sig_vector.connect(handler42);
    sig_vector.connect(handler777);
    std::vector<int> results = sig_vector.emit();
    const std::vector<int> reference = {777, 42, 1, 42, 777, };
    assert(results == reference);
  }
};

class TestCollectorUntil0 {
  bool check1, check2;
  TestCollectorUntil0() : check1(0), check2(0) {}
  bool handler_true() {
    check1 = true;
    return true;
  }
  bool handler_false() {
    check2 = true;
    return false;
  }
  bool handler_abort() { abort(); }

 public:
  static void run() {
    TestCollectorUntil0 self;
    Simple::Signal<bool(), Simple::CollectorUntil0<bool>> sig_until0;
    sig_until0.connect(Simple::slot(self, &TestCollectorUntil0::handler_true));
    sig_until0.connect(Simple::slot(self, &TestCollectorUntil0::handler_false));
    sig_until0.connect(Simple::slot(self, &TestCollectorUntil0::handler_abort));
    assert(!self.check1 && !self.check2);
    const bool result = sig_until0.emit();
    assert(!result && self.check1 && self.check2);
  }
};

class TestCollectorWhile0 {
  bool check1, check2;
  TestCollectorWhile0() : check1(0), check2(0) {}
  bool handler_0() {
    check1 = true;
    return false;
  }
  bool handler_1() {
    check2 = true;
    return true;
  }
  bool handler_abort() { abort(); }

 public:
  static void run() {
    TestCollectorWhile0 self;
    Simple::Signal<bool(), Simple::CollectorWhile0<bool>> sig_while0;
    sig_while0.connect(Simple::slot(self, &TestCollectorWhile0::handler_0));
    sig_while0.connect(Simple::slot(self, &TestCollectorWhile0::handler_1));
    sig_while0.connect(Simple::slot(self, &TestCollectorWhile0::handler_abort));
    assert(!self.check1 && !self.check2);
    const bool result = sig_while0.emit();
    assert(result == true && self.check1 && self.check2);
  }
};

static void bench_simple_signal() {
  Simple::Signal<void(void *, uint64_t)> sig_increment;
  sig_increment.connect(test_counter_add2);
  const uint64_t start_counter = TestCounter::get();
  const uint64_t benchstart = timestamp_benchmark();
  uint64_t i;
  for (i = 0; i < 999999; i++) {
    sig_increment.emit(0, 1);
  }
  const uint64_t benchdone = timestamp_benchmark();
  const uint64_t end_counter = TestCounter::get();
  assert(end_counter - start_counter == i);
  printf("OK\n  Benchmark: Simple::Signal: %fns per emission (size=%zu): ",
         size_t(benchdone - benchstart) * 1.0 / size_t(i),
         sizeof(sig_increment));
}

static void bench_callback_loop() {
  void (*counter_increment)(void *, uint64_t) = test_counter_add2;
  const uint64_t start_counter = TestCounter::get();
  const uint64_t benchstart = timestamp_benchmark();
  uint64_t i;
  for (i = 0; i < 999999; i++) {
    counter_increment(0, 1);
  }
  const uint64_t benchdone = timestamp_benchmark();
  const uint64_t end_counter = TestCounter::get();
  assert(end_counter - start_counter == i);
  printf("OK\n  Benchmark: callback loop: %fns per round: ",
         size_t(benchdone - benchstart) * 1.0 / size_t(i));
}

uint64_t TestCounter::get() { return test_counter_var; }

void TestCounter::set(uint64_t v) { test_counter_var = v; }

void TestCounter::add2(void *, uint64_t v) { test_counter_var += v; }

int main(int argc, char *argv[]) {
  printf("Signal/Basic Tests: ");
  BasicSignalTests::run();
  printf("OK\n");

  printf("Signal/CollectorVector: ");
  TestCollectorVector::run();
  printf("OK\n");

  printf("Signal/CollectorUntil0: ");
  TestCollectorUntil0::run();
  printf("OK\n");

  printf("Signal/CollectorWhile0: ");
  TestCollectorWhile0::run();
  printf("OK\n");

  printf("Signal/Benchmark: Simple::Signal: ");
  bench_simple_signal();
  printf("OK\n");

  printf("Signal/Benchmark: callback loop: ");
  bench_callback_loop();
  printf("OK\n");

  return 0;
}

#endif  // DISABLE_TESTS

// g++ -Wall -O2 -std=gnu++0x -pthread simplesignal.cc -lrt && ./a.out
