# stdcall with too many arguments

The repository's purpose is to demonstrate and explain why calling convention matters when calling a function with more
arguments than defined. Specifically, calling a `stdcall` function with too many arguments is harmful.

# Table of Contents
<!-- TOC -->
* [x86 calling conventions](#x86-calling-conventions)
* [Example program](#example-program)
* [Explanation](#explanation)
  * [`cdecl` stack](#cdecl-stack-)
  * [`stdcall` stack](#stdcall-stack-)
* [Fun part](#fun-part)
<!-- TOC -->

# x86 calling conventions

In 32-bit x86, there are two common calling conventions:

* `cdecl`:
    * default in C and Linux
    * parameters passed via the stack
    * **caller** cleans up the stack after return
* `stdcall`:
    * default in Windows APIs
    * parameters passed via the stack
    * **callee** cleans up the stack with return

# Example program

`f` is defined with one parameter in `f.c` but called with two arguments in `main.c` to demonstrate the effect of
calling conventions.

The example program can be compiled by executing `make` and then run using

* `./main c` to test `cdecl`
* `./main s` to test `stdcall`

When run with `cdecl`, nothing problematic happens. But when run with `stdcall` the program crashes with a segfault.

gcc arguments used:

* `-m32`: 32-bit mode because `cdecl` and `stdcall` are defined here. x64 has its own convention.
* `-no-pie -fno-pic`: no position-independent code to make disassembly more readable
* `-fomit-frame-pointer`: with frame pointer the code would not crash
* `-mpreferred-stack-boundary=2`: when calling a function, POSIX demands that the stack pointer is a multiple of a
  specific value. To achieve that, code is added before every call to ensure this. I have reduced this to 2**2 = 4 here
  to prevent those stack pointer instructions from being generated because it would make the disassembly less readable.
* `-O1`: Makes disassembly more readable. `-O2` made it a bit worse.

# Explanation

```bash
make 
gdb -q -batch -ex 'disas main_cdecl' -ex 'disas f_cdecl' ./main
gdb -q -batch -ex 'disas main_stdcall' -ex 'disas f_stdcall' ./main
```

The two assembly listings are next to each other for better comparability:

```
main_cdecl:                     main_stdcall:
    push   $123                     push   $123                # 1
    push   $456                     push   $456                # 2
    call   0x11e5 <f_cdecl>         call   0x11e2 <f_stdcall>  # 3
    add    $0x8,%esp                                           # 4
    ret                             ret                        # 5

f_cdecl:                        f_stdcall:
    ret                             ret    $0x4                # 6
```

In the **left listing** for the `cdecl` case, you can see that the two parameters are pushed, then `f_cdecl` is called,
which immediately `ret`urns, because it is an empty function. The `call` pushed the address of the `add` to the stack,
and the `ret` removed it from it and caused the program to continue from `add`. So, after the call, the stack still
contains the two `push`ed arguments. The `add` sets back the `%esp` register to its state before the two `push`es, and
hence everything is fine with the `cdecl`. This works because the caller, `main_cdecl`, is responsible for cleaning up
the stack. It has pushed more to the stack than necessary, but it knows that and can correctly set back `%esp` after the
call.

However, in the **right listing** for the `stdcall` case, the `add` is missing because in `stdcall`, it is not the
caller's (`main_stdcall`) responsibility to clean up the stack after a call. Instead, it is the callee's (`f_stdcall`)
responsibility. But the callee does not know how many arguments it has been called with. Only the caller can know that.
The callee only removes as many arguments from the stack as it expected to be called with, which is one. And hence there
is the `0x4` in `f_stdcall`'s `ret`. This kind of `ret`, in addition to what the `ret` does, also adjusts `%esp` by the
number specified. As a `push` in 32-bit x86 adds 4 bytes to the stack, `f_stdcall` returns with `ret $0x4`, to remove
the one 32-bit from the stack that it expected to have gotten passed. This means that after the return from `f_stdcall`
the `$123` is where `%esp` points at, and that is then where `main_stdcall`'s `ret` will return to, an address that is
hopefully not mapped to anything executable.

A look at the stack while the program is being executed is following.

## `cdecl` stack:

```
# Just entered `main_cdecl`:
  | ...                              |
  | return address to main           | <- %esp
  | ...                              |

# After #1, #2, #3, standing at #6
  | ...                              |
  | return address to main           |
  | $456                             |
  | $123                             |
  | return address to main_cdecl     | <- %esp 
  | ...                              |

# After #6, standing at #4
  | ...                              |
  | return address to main           |
  | $456                             | 
  | $123                             | <- %esp
  | return address to main_cdecl     | 
  | ...                              |
  
# After #4, standing at #5
  | ...                              |
  | return address to main           | <- %esp
  | $456                             | 
  | $123                             | 
  | return address to main_cdecl     | 
  | ...                              |
```

When `#5` gets executed, `%esp` points at the return address to main and can hence return to main. In the `stdcall`
stack is different.

## `stdcall` stack:

```
# Just entered `main_stdcall`:
  | ...                              |
  | return address to main           | <- %esp
  | ...                              |

# After #1, #2, #3, standing at #6
  | ...                              |
  | return address to main           |
  | $456                             |
  | $123                             |
  | return address to main_stdcall   | <- %esp 
  | ...                              |

# After #6, standing at #5
  | ...                              |
  | return address to main           |
  | $456                             | <- %esp      PROBLEM
  | $123                             | 
  | return address to main_cdecl     | 
  | ...                              |
```

Now, when `#5` gets executed, we have a problem because `%esp` does not point at the return address to main. Instead it
points at `$456` because `%esp` has only been adjusted by the callee by `$4` because it only expected a single argument.

We can abuse this situation by pushing a useful address to the stack instead of `$456`.

# Fun part

Instead of `$456` `(int)busyloop` is passed when `./main f` is executed, which causes the `busyloop` function to be
called, which loops forever. But, of course, any other function may be called as well, if useful.

GDB can be used to verify that it is the busyloop function running:

```bash
gdb -batch -x ./fun.gdb --args ./main f
```
