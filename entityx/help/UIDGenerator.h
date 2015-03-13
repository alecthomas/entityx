#pragma once

#include <type_traits>

namespace entityx {

class UIDGenerator final {
 public:
  typedef std::size_t Family;

  UIDGenerator(void) = delete;
  UIDGenerator(const UIDGenerator &) = delete;
  UIDGenerator &operator=(const UIDGenerator &) = delete;
  ~UIDGenerator(void) = delete;

  template <typename T>
  static typename std::enable_if<!std::is_const<T>::value && !std::is_volatile<T>::value, std::size_t>::type get_uid(void) {
    static const std::size_t uid = UIDGenerator::get_counter_and_increment();
    return uid;
  }

  template <typename T>
  static typename std::enable_if<std::is_const<T>::value || std::is_volatile<T>::value, std::size_t>::type get_uid(void) {
    return get_uid<std::remove_cv<T>::type>();
  }

 private:
  static inline std::size_t get_counter_and_increment(void) {
    static std::size_t counter = 0;
    return counter++;
  }
};

}