#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <utility>

namespace details {

template <typename Func> struct ScopeGuardImpl {
  Func f;
  explicit ScopeGuardImpl(Func &&fun) : f(::std::forward<Func>(fun)) {}
  ~ScopeGuardImpl() { f(); }
};

struct ScopeGuardDump {};

} // namespace details

template <typename Func>
auto operator*(::details::ScopeGuardDump /* dump */, Func &&fun)
    -> ::details::ScopeGuardImpl<Func> {
  return ::details::ScopeGuardImpl<Func>{::std::forward<Func>(fun)};
}

#define MAKE_SCOPE_GUARD_VAR_NAME_1(NAME, COUNT) NAME##COUNT

#define MAKE_SCOPE_GUARD_VAR_NAME_2(NAME, COUNT)                               \
  MAKE_SCOPE_GUARD_VAR_NAME_1(NAME, COUNT)

#define MAKE_SCOPE_GUARD                                                       \
  auto MAKE_SCOPE_GUARD_VAR_NAME_2(_details_scope_guard, __COUNTER__) =        \
      details::ScopeGuardDump{} *[&]

#endif // SCOPE_GUARD_HPP_
