#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <utility>

namespace _details {

template <typename Func> struct ScopeGuardImpl {
  Func f;
  ScopeGuardImpl(Func &&fun) : f(::std::forward<Func>(fun)) {}
  ~ScopeGuardImpl() { f(); }
};

struct ScopeGuardDump {};

} // namespace _details

template <typename Func>
auto operator*(::_details::ScopeGuardDump /* dump */, Func &&fun)
    -> ::_details::ScopeGuardImpl<Func> {
  return ::std::forward<Func>(fun);
}

#define _MAKE_SCOPE_GUARD_VAR_NAME_1(NAME, COUNT) NAME##COUNT

#define _MAKE_SCOPE_GUARD_VAR_NAME_2(NAME, COUNT)                              \
  _MAKE_SCOPE_GUARD_VAR_NAME_1(NAME, COUNT)

#define MAKE_SCOPE_GUARD                                                       \
  auto _MAKE_SCOPE_GUARD_VAR_NAME_2(_details_scope_guard, __COUNTER__) =       \
      _details::ScopeGuardDump{} *[&]

#endif // SCOPE_GUARD_HPP_
