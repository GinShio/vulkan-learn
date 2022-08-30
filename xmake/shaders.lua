rule("glsl")
    set_extensions(".vert", ".tesc", ".tese",
                   ".geom", ".frag", ".comp")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        import("lib.detect.find_tool")
        local compiler = assert(find_tool("glslc") or find_tool("glslangValidator"), "glsl compiler not found!")
        local targetfile = path.join(target:autogendir(), "shaders", path.filename(sourcefile) .. ".spv")

        local defaultflag = nil
        if compiler.name == "glslc" then
            if is_mode("debug") then
                defaltflag = {"-O0", "-g", table.unpack(defaultflag)}
            end
            defaultflag = {"--target-env=vulkan", table.unpack(defaultflag)}
        else
            if is_mode("debug") then
                defaltflag = {"-Od", table.unpack(defaultflag)}
            end
            defaultflag = {"--target-env", "vulkan1.0", "-V", table.unpack(defaultflag)}
        end

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling glsl %s", sourcefile)
        batchcmds:mkdir(path.directory(targetfile))
        batchcmds:vrunv(compiler.program, {"-o", targetfile, sourcefile, table.unpack(defaultflag)})
        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(targetfile))
        batchcmds:set_depcache(target:dependfile(targetfile))
    end)
