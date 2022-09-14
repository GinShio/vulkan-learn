target("VulkanBase")
    set_kind("static")
    add_files("base_type.cpp"
              ,"create.cpp"
              ,"window.cpp"
    )
    add_includedirs(path.join("$(projectdir)", "include"))
    add_includedirs(path.join("$(projectdir)", "third-party"))
    before_build_file(enable_clang_tidy)
    on_load(function (target)
            target:add(find_packages("vulkan", "sdl2"))
    end)
