set_project("vulkan-learn")
set_languages("cxx17")
set_warnings("all")
add_rules("mode.debug", "mode.release")

includes("xmake/common.lua"
         ,"xmake/shaders.lua"
         ,"xmake/tidy.lua"
)

common_option()

add_subdirs("src")
add_subdirs("projects")
