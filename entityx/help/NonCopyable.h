//  Inspired heavily by boost::noncopyable

#pragma once

namespace entityx {
namespace help {

class NonCopyable {
protected:
  NonCopyable() = default;
  ~NonCopyable() = default;


  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator = (const NonCopyable &) = delete;
};


}  // namespace help
}  // namespace entityx
