const std = @import("std");
const fs = std.fs;
const File = fs.File;

pub fn println(comptime msg: []const u8) void {
    std.debug.print(msg, .{});
    std.debug.print("\n", .{});
}

pub fn vprintln(comptime msg: []const u8, args: anytype) void {
    std.debug.print(msg, args);
    std.debug.print("\n", .{});
}

pub fn toAbsolutePath(allocator: std.mem.Allocator, path: []const u8) ![]const u8 {
    const cwd = std.process.getCwdAlloc(allocator) catch |err| {
        vprintln("Error getting the current directory error was {}", .{err});
        return err;
    };
    defer allocator.free(cwd);
    const full_path = fs.path.resolve(allocator, &[_][]const u8{ cwd, path }) catch |err| {
        vprintln("Failed to resolve relative path, error was {}", .{err});
        return err;
    };
    return full_path;
}

pub fn getFileContents(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    const full_path = try toAbsolutePath(allocator, path);
    defer allocator.free(full_path);
    const src_file = fs.openFileAbsolute(full_path, .{}) catch |err| {
        vprintln("Whoops, I can't seem to open {s}, error was {}.", .{ path, err });
        return err;
    };
    const stats = src_file.stat() catch |err| {
        vprintln("Whoops, I can't seem to stat {s}, error was {}.", .{ path, err });
        return err;
    };

    // Allocate a buffer big enough to hold the whole file.
    var buffer = allocator.alloc(u8, stats.size) catch |err| {
        vprintln("Error attempting to allocate enough memory, error was: {}", .{err});
        return err;
    };

    var bytes_read = src_file.readAll(buffer) catch |err| {
        vprintln("Failed to read data from {s}, error was {}.", .{ path, err });
        return err;
    };
    if (bytes_read != stats.size) {
        vprintln("File read size mismatch, expected to read {}, actually read {}.", .{ stats.size, bytes_read });
        return error.SizeMismatch;
    }

    return buffer;
}

pub fn main() void {
    // Setup an allocator with all the safety and logging flags set.
    var gpa = std.heap.GeneralPurposeAllocator(.{ .safety = true, .enable_memory_limit = true, .never_unmap = true, .retain_metadata = true, .verbose_log = true }){};
    defer _ = gpa.deinit();
    gpa.setRequestedMemoryLimit(1000000);
    var allocator = gpa.allocator();

    const args = std.process.argsAlloc(allocator) catch unreachable;
    defer std.process.argsFree(allocator, args);

    if (args.len != 2) {
        switch (args.len) {
            1 => println("Uhoh, please specify a source file to work on."),
            else => vprintln("Hmm, I can only take one source file at the moment.  Got {}.\n", .{args.len - 1}),
        }
        return;
    }

    const src = getFileContents(allocator, args[1]) catch unreachable;
    defer allocator.free(src);
    println("I was able to read the file!");
    println("Look, here it is:");
    vprintln("{s}\n", .{src});
}
