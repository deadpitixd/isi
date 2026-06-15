# ILS: Grammar & Syntax Rules
Version: 1.0<br>
Status: Authoritative

This document defines the formal lexical tokens and syntax structures of the Interscript Interpreted (ISI) language.

---

## 1. Data Types and Keywords
The following keywords are reserved by the engine and cannot be used as variable identifiers:

*   **Primitive Types:** `int`, `float`, `char`, `string`, `lib`
*   **Control Flow:** `print`, `import`, `extern`

## 2. Structural Syntax Rules

### 2.1 Variable Declaration
Variables must be explicitly typed upon declaration.
```cpp
int myVar = 10;
string name = "isi";
```
Variables cannot start with numbers, but they can contain them
```cpp
// this is incorrect
int 1var = 123;
// this is correct
int var1 = 123;
````