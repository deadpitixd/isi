# ILS: Code Execution
Version: 1.0<br>
Status: Draft

This document specifies the way of executing 
ISI code.

## 1. JIT
ISI should use JIT (Just in Time) compilation,
but currently uses Byte Code.

The programmer should have access to JIT and 
AOT compilation.

## 2. AOT
ISI should compile directly to C++, which gets
compiled with -o3 and ISIs C++ version.

## 3. Combining
The programmer should have access to both.

JIT would essentially be a way of quickly testing
code, it shouldn't be used for releasing the finished code.

AOT should be the default way of releasing the finished product,<br>
as it uses C++ compilation, which should be the way of creating executables.