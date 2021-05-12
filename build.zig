// Project-Specific Options:
//   -Drelease-safe=[bool]       Optimizations on and safety on
//   -Drelease-fast=[bool]       Optimizations on and safety off
//   -Drelease-small=[bool]      Size optimizations on and safety off
// zig build --color on -Drelease-fast=false
const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    // cl main.c /Zi /Fe".\bin\main.exe" /nologo /link opengl32.lib user32.lib gdi32.lib
    const exe = b.addExecutable("main", null);
    // c17 because I was originally compiling with cl.exe which has a minimum supported version of c17 for god knows why
    exe.setBuildMode(b.standardReleaseOptions());
    exe.addCSourceFile("main.c", &[_][]const u8{"-std=c17"});
    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("opengl32");
    exe.linkSystemLibrary("user32");
    exe.linkSystemLibrary("gdi32");
    // Actually generating the executable
    exe.install();

    b.default_step.dependOn(&exe.step);
    const run_cmd = exe.run();
}