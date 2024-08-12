add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("levilamina")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("vanish")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_defines("NOMINMAX", "UNICODE")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_packages("levilamina")
    add_shflags("/DELAYLOAD:bedrock_server.dll")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++23")
    set_symbols("debug")

    after_build(function (target)
        local mod_packer = import("scripts.after_build")

        local mod_define = {
            modName = target:name(),
            modFile = path.filename(target:targetfile())
        }
        
        mod_packer.pack_mod(target,mod_define)
    end)
