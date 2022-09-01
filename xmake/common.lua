function common_option()
  if is_plat("windows") then
    add_defines("SDL_MAIN_HANDLED")
  end

  if is_mode("debug") then
    add_defines("DEBUG")
  elseif is_mode("release") then
    add_defines("NDEBUG")
  end

  if is_host("windows") and not is_subhost("msys", "cygwin") then
  else
    add_cxxflags("-fno-stack-protector"
                 ,"-fno-common"
                 ,"-Wall"
                 ,"-march=native"
    )
    if is_mode("debug") then
      add_cxxflags("-fno-inline"
                   ,"-Weffc++"
                   -- from lua Makefile
                   ,"-Wdisabled-optimization"
                   ,"-Wdouble-promotion"
                   ,"-Wextra"
                   ,"-Wmissing-declarations"
                   ,"-Wredundant-decls"
                   ,"-Wshadow"
                   ,"-Wsign-compare"
                   ,"-Wundef"
                   -- ,"-Wfatal-errors"
      )
    end
  end
end
