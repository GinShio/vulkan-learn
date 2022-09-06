target("VulkanTexture")
    set_kind("binary")
    add_rules("glsl")
    add_deps("VulkanBase")
    add_files("main.vert"
              ,"main.frag"
    )
    add_files("main.cpp", "texture.cpp")
    add_includedirs(path.join("$(projectdir)", "include"))
    before_build_file(enable_clang_tidy)
    on_load(function (target)
            target:add(find_packages("vulkan", "sdl2"))
    end)
