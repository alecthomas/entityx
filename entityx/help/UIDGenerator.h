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
  static typename std::enable_if<!std::is_const<T>::value && !std::is_volatile<T>::value, std::size_t>::type GetUID(void) {
	static const std::size_t UID = UIDGenerator::GetCounterAndIncrement();
	return UID;
  }

  template <typename T>
  static typename std::enable_if<std::is_const<T>::value || std::is_volatile<T>::value, std::size_t>::type GetUID(void) {
	return GetUID<std::remove_cv<T>::type>();
  }

 private:
  static inline std::size_t GetCounterAndIncrement(void) {
	static std::size_t Counter = 0;
	return Counter++;
  }
};

}