# ILS: String formats
Version: 1.0<br>
Status: Draft

This document declares the rules for string formatting.

---
## 1. Format Strings
Format strings parse code inside curly braces that are placed inside them.

The code should return an string, or an object with a `.toString()` function, or one that can be expressed as a string.

Format strings are expressed by putting '`f`' behind quotation marks.
### 1.1 Example
```cpp
const int x = 25;
const int y = 50;
string format = f"Result of x*y: {x * y}";
```
The result here would be: "Result of x*y: 1250"

## 2. Variable Strings
Variable strings parse variables inside curly braces.

The variables should be a basic data type, or an object with a `.toString()` function.

Variable strings are expressed by putting '`v`' behind quotation marks.
### 2.1 Constrains
The Variable strings cannot express code, they only can express variables.

```cpp
string format = v"Result of a math operation: {194 * 107}";
// This is incorrect because the contents are not a variable.
int x = 6;
string format = v"X + 8: {x + 8}";
                            ~~~
// This is incorrect because v strings cannot parse their insides.
```
### 2.2 Example
```cpp
float pi = 3.141;
string format = v"PI: {pi}";
// This returns "PI: 3.141"
```
## 3. Not triggering the curly braces
To prevent the interpreter from parsing anything in these braces, prefix them with a backslash `\`

### 3.1 Example
```cpp
int age = 19;
string format = f"I am {age}, and this is a curly brace \{, \}";
// Output: I am 19, and this is a curly brace { }
```

