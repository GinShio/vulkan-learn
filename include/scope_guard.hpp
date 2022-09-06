#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <utility>

namespace details {

template <typename Func> struct ScopeGuardImpl : public Func {
  explicit ScopeGuardImpl(Func &&fun) : Func(::std::forward<Func>(fun)) {}
  ~ScopeGuardImpl() { this->Func::operator()(); }
};

struct ScopeGuardDummy {};

} // namespace details

template <typename Func>
auto operator*(::details::ScopeGuardDummy /* dump */, Func &&fun)
    -> ::details::ScopeGuardImpl<Func> {
  return ::details::ScopeGuardImpl<Func>{::std::forward<Func>(fun)};
}

#define MAKE_SCOPE_GUARD_VAR_NAME_1(NAME, COUNT) NAME##COUNT

#define MAKE_SCOPE_GUARD_VAR_NAME_2(COUNT)                                     \
  MAKE_SCOPE_GUARD_VAR_NAME_1(_details_scope_guard, COUNT)

#define MAKE_SCOPE_GUARD                                                       \
  auto MAKE_SCOPE_GUARD_VAR_NAME_2(__COUNTER__) =                              \
      details::ScopeGuardDummy{} *[&]() mutable

#endif // SCOPE_GUARD_HPP_
