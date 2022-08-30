target("VulkanBase")
    set_kind("static")
    add_files("window.cpp"
              ,"create.cpp"
    )
    add_includedirs(path.join("$(projectdir)", "include"))
    before_build_file(enable_clang_tidy)
    on_load(function (target)
            target:add(find_packages("vulkan", "sdl2"))
    end)
