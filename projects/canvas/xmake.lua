target("VulkanCanvas")
    set_kind("binary")
    add_rules("glsl")
    add_deps("VulkanBase")
    add_files("main.vert"
              ,"shader/helloworld.frag"
              ,"shader/circle.frag"
    )
    add_files("main.cpp", "canvas.cpp")
    add_includedirs(path.join("$(projectdir)", "include"))
    on_load(function (target)
            target:add(find_packages("vulkan", "sdl2"))
    end)
