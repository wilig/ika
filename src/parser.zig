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

fn parse(allocator: std.mem.Allocator, tokens: *mem.TokenIterator(u8), min_prec: u8) *AstNode {
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
