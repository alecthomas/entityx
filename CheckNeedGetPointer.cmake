cmake_minimum_required(VERSION 2.8.3)

check_cxx_source_compiles(
"
#include <boost/get_pointer.hpp>

namespace boost {
template <class T> inline T * get_pointer(const std::shared_ptr<T> &p) {
  return p.get();
}
}

int main() {
  return 0;
}
"
ENTITYX_NEED_GET_POINTER_SHARED_PTR_SPECIALIZATION
)
