const std = @import("std");

pub fn println(comptime msg: []const u8) void {
    std.debug.print(msg, .{});
    std.debug.print("\n", .{});
}

pub fn main() void {
    var buf: [1024]u8 = undefined;
    const alloc = std.heap.FixedBufferAllocator.init(buf[0..]).allocator();
    const args = std.process.argsAlloc(alloc) catch unreachable;
    defer std.process.argsFree(alloc, args);

    if (args.len != 2) {
        switch (args.len) {
            1 => println("Uhoh, please specify a source file to work on."),
            else => std.debug.print("Hmm, I can only take one source file at the moment.  Got {}.\n", .{args.len - 1}),
        }
        return;
    }

    std.debug.print("One day I will actually processs: {s}\n", .{args[1]});
}
