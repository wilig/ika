const std = @import("std");
const mem = std.mem;
const panic = std.debug.panic;
const testing = std.testing;

const TokenType = enum {
    atom,
    addition,
    subtraction,
    multiplication,
    division,
    eof,
};

const TokenTypeMap = std.ComptimeStringMap(TokenType, .{
    .{ "+", .addition },
    .{ "-", .subtraction },
    .{ "*", .multiplication },
    .{ "/", .division },
});

const AstNode = struct {
    const Self = @This();

    value: []const u8,
    _type: TokenType,
    lhs: ?*AstNode = null,
    rhs: ?*AstNode = null,

    pub fn init(allocator: std.mem.Allocator, value: []const u8, _type: TokenType, lhs: ?*AstNode, rhs: ?*AstNode) !*Self {
        var node = try allocator.create(Self);
        errdefer allocator.destroy(node);
        node.* = .{ .value = value, ._type = _type, .lhs = lhs, .rhs = rhs };
        return node;
    }

    pub fn deinit(self: *Self, allocator: std.mem.Allocator) void {
        if (self.lhs) |lhs| {
            lhs.deinit(allocator);
        }
        if (self.rhs) |rhs| {
            rhs.deinit(allocator);
        }
        allocator.destroy(self);
    }

    pub fn toSexp(self: *Self, allocator: std.mem.Allocator) ![]u8 {
        var buffer = std.ArrayList(u8).init(allocator);
        self.buildSexp(&buffer);
        return buffer.toOwnedSlice();
    }

    pub fn buildSexp(self: *Self, buffer: *std.ArrayList(u8)) void {
        if (self._type != .atom) {
            buffer.writer().print("(", .{}) catch @panic("Can't build sexp.");
            buffer.writer().print("{s} ", .{self.value}) catch @panic("Can't build sexp");
        } else {
            buffer.writer().print("{s}", .{self.value}) catch @panic("Can't build sexp");
        }
        if (self.lhs) |lhs| {
            lhs.buildSexp(buffer);
        }
        if (self.rhs) |rhs| {
            buffer.writer().print(" ", .{}) catch @panic("Can't build sexp.");
            rhs.buildSexp(buffer);
        }
        if (self._type != .atom) {
            buffer.writer().print(")", .{}) catch @panic("Can't build sexp");
        }
    }
};

pub fn tokenize(input: []const u8) *mem.TokenIterator(u8) {
    return &mem.tokenize(u8, input, " \n\t");
}

fn determineType(value: []const u8) TokenType {
    if (TokenTypeMap.get(value)) |op| {
        return op;
    } else {
        return TokenType.atom;
    }
}

pub fn parse(allocator: std.mem.Allocator, tokens: *mem.TokenIterator(u8), min_prec: u8) *AstNode {
    const token = tokens.next() orelse panic("Out of tokens!", .{});
    var tokenType: TokenType = determineType(token);
    var lhs: *AstNode = switch (tokenType) {
        else => panic("Unexpected token '{s}' not allowed here.", .{token}),
        .atom => AstNode.init(allocator, token, tokenType, null, null) catch panic("Couldn't allocate a node for {s}", .{token}),
    };

    while (true) {
        var operator = tokens.peek() orelse break;
        tokenType = determineType(operator);
        if (tokenType == .atom) {
            panic("Expected an numeric operator here, got '{s}' instead", .{operator});
        }

        var operator_precedence = @enumToInt(tokenType);
        if (operator_precedence < min_prec) {
            break;
        }

        operator = tokens.next() orelse unreachable;
        var rhs = parse(allocator, tokens, operator_precedence + 1);

        var orig_lhs = lhs;

        lhs = AstNode.init(allocator, operator, tokenType, orig_lhs, rhs) catch @panic("Couldn't create node");
    }

    return lhs;
}

const LLVMTag = enum { op, val };

const LLVMNode = struct {
    const Self = @This();
    allocator: std.mem.Allocator,
    value: []const u8,
    reg_num: u32,
    kind: LLVMTag,
    lhs: ?*LLVMNode = null,
    rhs: ?*LLVMNode = null,

    pub fn init(allocator: std.mem.Allocator, kind: LLVMTag, value: []const u8, reg_num: u32) !*Self {
        var instance = try allocator.create(Self);
        errdefer allocator.destroy(instance);
        instance.* = .{ .allocator = allocator, .value = value, .kind = kind, .reg_num = reg_num };
        return instance;
    }

    pub fn deinit(self: *Self) void {
        if (self.lhs) |lhs| {
            lhs.deinit();
        }
        if (self.rhs) |rhs| {
            rhs.deinit();
        }
        self.allocator.destroy(self);
    }
};

pub const LLVM_IR = struct {
    const Self = @This();

    const prelude =
        \\; ModuleID = 'int.ika'
        \\source_filename = "int.ika"
        \\target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
        \\target triple = "x86_64-redhat-linux-gnu"
        \\
        \\@.str = private unnamed_addr constant [12 x i8] c"Answer: %d\0A\00", align 1
        \\
        \\define dso_local i32 @main() #0 {
        \\
    ;

    const postfix =
        \\  ret i32 0
        \\}
        \\
        \\declare dso_local i32 @printf(i8* noundef, ...) #1
        \\
        \\!0 = !{i32 1, !"wchar_size", i32 4}
        \\!1 = !{i32 7, !"uwtable", i32 1}
        \\!2 = !{i32 7, !"frame-pointer", i32 2}
        \\!3 = !{!"ika version 0.0.1 (Linux)"}
    ;

    allocator: std.mem.Allocator,
    reg_counter: u32 = 0,

    pub fn init(alloc: std.mem.Allocator) Self {
        return Self{
            .allocator = alloc,
        };
    }

    pub fn generate(self: *Self, root: *AstNode) ![]u8 {
        var llvm_ir = std.ArrayList(u8).init(self.allocator);
        defer llvm_ir.deinit();
        try llvm_ir.appendSlice(LLVM_IR.prelude);
        const llvm_root_node = try self.translate(root);
        defer llvm_root_node.deinit();
        const reg = self.appendNode(&llvm_ir, llvm_root_node);
        const println = try std.fmt.allocPrint(self.allocator, "%noop = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str, i64 0, i64 0), i32 noundef %IKA{})\n", .{reg});
        defer self.allocator.free(println);
        try llvm_ir.appendSlice(println);
        try llvm_ir.appendSlice(LLVM_IR.postfix);
        return llvm_ir.toOwnedSlice();
    }

    fn appendNode(self: *Self, buf: *std.ArrayList(u8), node: *LLVMNode) !u32 {
        var buffer: [100]u8 = std.mem.zeroes([100]u8);
        switch (node.kind) {
            .op => {
                const lhs_reg = self.appendNode(buf, node.lhs.?);
                const rhs_reg = self.appendNode(buf, node.rhs.?);
                try buf.appendSlice(try std.fmt.bufPrint(&buffer, "%IKA{} = {s} %IKA{}, %IKA{}\n\n", .{ node.reg_num, node.value, lhs_reg, rhs_reg }));
                return node.reg_num;
            },
            .val => {
                try buf.appendSlice(try std.fmt.bufPrint(&buffer, "%IKA{} = alloca i32, align 4\n", .{node.reg_num}));
                try buf.appendSlice(try std.fmt.bufPrint(&buffer, "store i32 {s}, i32* %IKA{}, align 4\n", .{ node.value, node.reg_num }));
                try buf.appendSlice(try std.fmt.bufPrint(&buffer, "%IKA{} = load i32, i32* %IKA{}, align 4\n\n", .{ node.reg_num + 1, node.reg_num }));
                return node.reg_num + 1;
            },
        }
    }

    fn translate(self: *Self, node: *AstNode) !*LLVMNode {
        self.reg_counter += 1;
        if (node._type != TokenType.atom) {
            const value =
                switch (node._type) {
                TokenType.addition => "add nsw i32",
                TokenType.subtraction => "sub i32",
                TokenType.multiplication => "mul nsw i32",
                TokenType.division => "sdiv i32",
                else => unreachable,
            };
            var llvm_node = try LLVMNode.init(self.allocator, LLVMTag.op, value, self.reg_counter);
            if (node.lhs) |left| {
                llvm_node.lhs = self.translate(left) catch unreachable;
            }
            if (node.rhs) |right| {
                llvm_node.rhs = self.translate(right) catch unreachable;
            }

            return llvm_node;
        } else {
            var llvm_node = try LLVMNode.init(self.allocator, LLVMTag.val, node.value, self.reg_counter);
            // Leave room for the load register
            self.reg_counter += 1;

            return llvm_node;
        }
    }
};

test "simple numeric parsing tests" {
    const alloc = std.testing.allocator;

    var one = "1";
    var first = parse(alloc, tokenize(one), 0);
    defer first.deinit(alloc);
    var firstSexp = try first.toSexp(alloc);
    defer alloc.free(firstSexp);
    try testing.expect(mem.eql(u8, firstSexp, "1"));

    var two = "10 + 20";
    var second = parse(alloc, tokenize(two), 0);
    defer second.deinit(alloc);
    var secondSexp = try second.toSexp(alloc);
    defer alloc.free(secondSexp);
    try testing.expect(mem.eql(u8, secondSexp, "(+ 10 20)"));

    const three = "1 + 5 * 8";
    var third = parse(alloc, tokenize(three), 0);
    defer third.deinit(alloc);
    var thirdSexp = try third.toSexp(alloc);
    defer alloc.free(thirdSexp);
    try testing.expect(mem.eql(u8, thirdSexp, "(+ 1 (* 5 8))"));

    const four = "1 + 5 * 8 / 2 - 12";
    var fourth = parse(alloc, tokenize(four), 0);
    defer fourth.deinit(alloc);
    var fourthSexp = try fourth.toSexp(alloc);
    defer alloc.free(fourthSexp);
    try testing.expect(mem.eql(u8, fourthSexp, "(+ 1 (- (* 5 (/ 8 2)) 12))"));
}

test "simple llvm ir code generation" {
    const expected_result =
        \\; ModuleID = 'int.ika'
        \\source_filename = "int.ika"
        \\target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
        \\target triple = "x86_64-redhat-linux-gnu"
        \\
        \\@.str = private unnamed_addr constant [12 x i8] c"Answer: %d\0A\00", align 1
        \\
        \\define dso_local i32 @main() #0 {
        \\%IKA3 = alloca i32, align 4
        \\store i32 10, i32* %IKA3, align 4
        \\%IKA4 = load i32, i32* %IKA3, align 4
        \\
        \\%IKA5 = alloca i32, align 4
        \\store i32 3, i32* %IKA5, align 4
        \\%IKA6 = load i32, i32* %IKA5, align 4
        \\
        \\%IKA2 = sdiv i32 %IKA4, %IKA6
        \\
        \\%IKA7 = alloca i32, align 4
        \\store i32 100, i32* %IKA7, align 4
        \\%IKA8 = load i32, i32* %IKA7, align 4
        \\
        \\%IKA1 = add nsw i32 %IKA2, %IKA8
        \\
        \\%noop = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str, i64 0, i64 0), i32 noundef %IKA1)
        \\  ret i32 0
        \\}
        \\
        \\declare dso_local i32 @printf(i8* noundef, ...) #1
        \\
        \\!0 = !{i32 1, !"wchar_size", i32 4}
        \\!1 = !{i32 7, !"uwtable", i32 1}
        \\!2 = !{i32 7, !"frame-pointer", i32 2}
        \\!3 = !{!"ika version 0.0.1 (Linux)"}
    ;

    const alloc = std.testing.allocator;
    var stmt = "10 / 3 + 100";
    var ast_node = parse(alloc, tokenize(stmt), 0);
    defer ast_node.deinit(alloc);
    var llvm_ir = LLVM_IR.init(alloc);
    const ir_code = try llvm_ir.generate(ast_node);
    defer alloc.free(ir_code);
    try std.testing.expect(std.mem.eql(u8, ir_code, expected_result));
}
