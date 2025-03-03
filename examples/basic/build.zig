const std = @import("std");
const B = std.Build;

pub fn build(b: *B) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "zig-exe-template",
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibCpp();

    exe.addIncludePath(b.path("../../"));
    exe.addCSourceFile(.{
        .file = b.path("./main.cpp"),
        .flags = &[_][]const u8{"-std=c++17"},
    });

    b.installArtifact(exe);
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
