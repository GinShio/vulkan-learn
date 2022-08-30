function enable_clang_tidy(target, sourcefile, opt)
  if not is_mode("debug") then
    return
  end
  local ext = path.extension(sourcefile)
  import("core.language.language")
  local lang = language.load_sk("cxx")
  local is_cxx = false
  for _, v in ipairs(lang:sourcekinds()["cxx"]) do
    if v == ext then
      is_cxx = true
      break
    end
  end
  if not is_cxx then
    return
  end
  import("lib.detect.find_tool")
  local tidy = assert(find_tool("clang-tidy"), "clang-tidy not found!")
  os.execv(tidy.program, {"-header-filter=\".*\"", sourcefile})
end
