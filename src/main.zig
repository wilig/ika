const std = @import("std");
const fs = std.fs;
const File = fs.File;
const parser = @import("parser.zig");

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

pub fn getFileContents(allocator: std.mem.Allocator, path: []const u8) ![]const u8 {
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

pub fn main() !void {
    // Setup an allocator with all the safety and logging flags set.
    var gpa = std.heap.GeneralPurposeAllocator(.{ .safety = true, .enable_memory_limit = true, .never_unmap = true, .retain_metadata = true }){};
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

    var start = std.time.milliTimestamp();
    const src = getFileContents(allocator, args[1]) catch |err| {
        switch (err) {
            error.FileNotFound => vprintln("File '{s}' does not exist.", .{args[1]}),
            error.OutOfMemory => println("Ran out of available memory reading the file, time to refactor."),
            else => vprintln("Compile error, reading source file: {}", .{err}),
        }
        return;
    };
    vprintln("Loading file took {}ms", .{std.time.milliTimestamp() - start});
    defer allocator.free(src);
    start = std.time.milliTimestamp();
    var ast_node = parser.parse(allocator, parser.tokenize(src), 0);
    defer ast_node.deinit(allocator);
    vprintln("Parsing took {}ms", .{std.time.milliTimestamp() - start});
    start = std.time.milliTimestamp();
    var llvm_ir = parser.LLVM_IR.init(allocator);
    const llvm_ir_code = llvm_ir.generate(ast_node) catch |err| {
        vprintln("Failed to generate LLVM ir code. Error: {}", .{err});
        return;
    };
    defer allocator.free(llvm_ir_code);
    vprintln("LLVM IR generation took {}ms", .{std.time.milliTimestamp() - start});

    start = std.time.milliTimestamp();
    var llc = std.ChildProcess.init(&[_][]const u8{"llc"}, allocator);
    llc.stdin_behavior = .Pipe;
    llc.stdout_behavior = .Pipe;
    llc.stderr_behavior = .Pipe;
    llc.spawn() catch {
        println("Failed to spawn llc");
        return;
    };
    _ = llc.stdin.?.writer().writeAll(llvm_ir_code) catch {
        println("Failed to write to stdin of process");
        return;
    };
    llc.stdin.?.close();
    // Critical for unknown reasons..
    llc.stdin = null;

    var llc_out = try llc.stdout.?.reader().readAllAlloc(allocator, std.math.maxInt(usize));
    defer allocator.free(llc_out);

    switch (try llc.wait()) {
        .Exited => |code| if (code != 0) {
            println("Bad exit code from llc");
        },
        else => unreachable,
    }
    vprintln("LLC translating took {}ms", .{std.time.milliTimestamp() - start});

    start = std.time.milliTimestamp();
    var gcc = std.ChildProcess.init(&[_][]const u8{ "gcc", "-o", "a.out", "-xassembler", "-" }, allocator);
    gcc.stdin_behavior = .Pipe;
    gcc.stdout_behavior = .Pipe;
    gcc.stderr_behavior = .Pipe;
    gcc.spawn() catch {
        println("Failed to spawn gcc");
        return;
    };
    _ = gcc.stdin.?.writer().writeAll(llc_out) catch {
        println("Failed to write to stdin of gcc process");
        return;
    };
    gcc.stdin.?.close();
    // Critical for unknown reasons..
    gcc.stdin = null;

    switch (try gcc.wait()) {
        .Exited => |code| if (code != 0) {
            println("Bad exit code from gcc");
        },
        else => unreachable,
    }
    vprintln("GCC ELF encoding took {}ms", .{std.time.milliTimestamp() - start});
}
