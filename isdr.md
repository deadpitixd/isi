# ISI Standard Code Ruleset


|table of contents|
-------------------
|What is ISCR|
|When to use ISCR|

## What is ISCR?
ISCR is a ruleset for ISI programmers to code in an efficient and readable way.<br>

## When to use ISCR?
Mostly use it while programming some important library.<br>
The <span style="color:lime;">ISI Standard Library</span> must follow these rules.
## Ruleset
### Variable annotations
1. Always use `const` if the variable shouldn't change<br>
eg.  `const float pi = 3.141;`
2. Never use spaces for indentation.
    - You should use 'Tab' characters
3. A line should not exceed 76 characters.
4. External functions should always use constant ids<br>
eg.:
```cpp
const int id = loadLibrary("lib");
extern(id) string doSomething();
//#And not#
loadLibrary("lib");
extern(0) string doSomething();
```
5. Code should work on the 4 previous versions, unless
major changes were added ie.<br>
Functions refactor, more efficient ways of doing the same thing.