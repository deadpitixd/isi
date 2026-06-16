# ILS: Specifier Keywords
Version: 1.0<br>
Status: Proposed

Specifier keywords give certain properties to objects that accept them.

## 1. `const`
`const` declares that a variable is constant, it cannot change.
If the program tries to modify a constant variable, the interpreter should throw an syntax error.

## 1.1 Constant optimizations
This will be explained with this code:
```cpp
const int x = 5;
print(x,"\n");
```
The code simply prints x out.

> Following bytecode results are from ISI Beta 1½

The bytecode for non optimized constants is this:
```
OP_PUSH   5
OP_STORE  1
OP_LOAD   1
OP_PRINT  
OP_PUSH   \n
OP_PRINT  
OP_PUSH   0
OP_HALT   
```
As you can see, there are unnecessary OP_LOAD Op Codes added, they
should be replaced with <br>`OP_LOAD [constant value]`

Optimized bytecode should be:
```
OP_PUSH   5
OP_PRINT  
OP_PUSH   \n
OP_PRINT  
OP_PUSH   0
OP_HALT   
```
As you can see, OP_STORE was removed and all OP_LOAD that instances pointed to `int x` were replaced by<br>
`OP_PUSH 5`

Constant variables should not be saved, since all their occurences are changed to a literal.

## 2. Overload
`overload` is a keyword, that tells the interpreter, that it serves the purpose of a default function.

Only interpreter functions can be overloaded.

## 3. extern()
Extern is a keyword that is placed before a function declaration, it states<br>
<span style="color:orange">"This function is declared in an C++ file with id [x]"</span>.<br>

Extern uses constant int ids, that point to files loaded by 'loadLibary()'