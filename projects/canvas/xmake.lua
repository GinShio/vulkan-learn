target("VulkanCanvas")
    set_kind("binary")
    add_rules("glsl")
    add_deps("VulkanBase")
    add_files("main.vert"
              ,"shader/helloworld.frag"
              ,"shader/circle.frag"
              ,"shader/pacman.frag"
    )
    add_files("main.cpp", "canvas.cpp")
    add_includedirs(path.join("$(projectdir)", "include"))
    before_build_file(enable_clang_tidy)
    on_load(function (target)
            target:add(find_packages("vulkan", "sdl2"))
    end)
