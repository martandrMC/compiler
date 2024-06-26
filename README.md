# Experimental Compiler
This project is an attempt to explore the landscape of compilers. It is loosely related to my main project, [PHINIX+](https://github.com/phinixplus)
as a potential language to backend to it, but has managed to become the most actively worked on project of my development "career" as of yet.
You can catch up with my future plans with this project by taking a look at the [Trello board](https://trello.com/b/EaTRZV1Y/compiler).

## The Why
> Science isn't about WHY. It's about WHY NOT. \
> \- Cave Johnson

In all seriousness, this is mostly an educational endeavor for myself, and a good programming practice. Don't expect the code to be
"enterprise grade", whatever that even means. It's a hobby project with the aspiration of being useful.

## The How
This programming language, which is as of yet left unnamed, will be mostly inspired by a few of my most beloved programming languages.
I will attempt to take the parts I most like out of each language and combine them in some way into this one language. Those languages being:
- **C**: Low level control (No-questions-asked memory access and minimal runtime)
- **Lua**: Expressive and readable syntax (`do`-`end`-style statements)
- **Rust**: Elegant and safe type system (The concept of a bottom type and more)
- **Zig**: The concept of no hidden functionality and also `T?` optionals
- **D**: Separation of `delegate`s from `function` pointers

### Examples
Printing the first 10 numbers of the Fibonacci Sequence:
```
var nat i = 0;
var nat a = 1, b = 0, c = 0;
while i < 10: do
	write(c);
	c = a + b;
	a = b;
	b = c;
	i += 1;
end
```

Demonstration of `if` cases acting as (limited) expressions:
```
var nat x = 5;
var nat y = if x > 3: 3 else x end
write(y); // 3
```

## The Bottom Line
Since I have been historically not *the best*<sup>TM</sup> at writing compilers, the initial stage of this project is to get a working
minimum viable product and only thereafter expand the feature set. That essentially means:
- Just three (plus two internal) types: `nat`, `int`, `bool`, `nil` (internal), `never` (internal)
- Just the basic statements: `if`-`elif`-`else`, `while`, `do`-`end`, `return` (not what you think it does)
- No type "decorators" like `const` or `ref` (pointers) or `?` (optionals)
- No I/O except for temporary intrinsic functions `read` and `write`
- No `func`s (function declarations)
- No multi-file support

All of these decisions were made in the effort to reduce the time needed to get to a working compiler. Once that happens, these features
will gradually get introduced into the language to complete it. It's all free reign from there on out.
