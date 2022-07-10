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
    const cwd = try std.process.getCwdAlloc(allocator);
    defer allocator.free(cwd);

    return try fs.path.resolve(allocator, &[_][]const u8{ cwd, path });
}

pub fn getFileContents(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    const full_path = try toAbsolutePath(allocator, path);
    defer allocator.free(full_path);
    const src_file = try fs.openFileAbsolute(full_path, .{});
    const stats = try src_file.stat();

    // Allocate a buffer big enough to hold the whole file.
    var buffer = try allocator.alloc(u8, stats.size);

    var bytes_read = try src_file.readAll(buffer);

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

    const args = std.process.argsAlloc(allocator) catch |err| {
        vprintln("Failed to correctly receive arguments!  Error was: {}", .{err});
        return;
    };
    defer std.process.argsFree(allocator, args);

    if (args.len != 2) {
        switch (args.len) {
            1 => println("Uhoh, please specify a source file to work on."),
            else => vprintln("Hmm, I can only take one source file at the moment.  Got {}.\n", .{args.len - 1}),
        }
        return;
    }

    const src = getFileContents(allocator, args[1]) catch |err| {
        switch (err) {
            error.FileNotFound => vprintln("File '{s}' does not exist.", .{args[1]}),
            error.OutOfMemory => println("Ran out of available memory reading the file, time to refactor."),
            else => vprintln("Compile error, reading source file: {}", .{err}),
        }
        return;
    };
    defer allocator.free(src);
    println("I was able to read the file!");
    println("Look, here it is:");
    vprintln("{s}\n", .{src});
}
