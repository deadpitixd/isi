# ILS: Functions
Version: 1.0<br>
Status: Proposed

## 1. Definition
Functions are reusable blocks of code that perform specific tasks.

Functions take in parameters and return values if they are not '`void`'

They are declared by:<br>
`[Returning Data Type] [Function Name] ([Parameter Type] [Param Name]) { [Code] }`

In code, it would look like this:
```cpp
int add(int a, int b){
    return a + b;
}
```

Functions can only return the type they were defined with.

## 2. Behaviour

1. Functions cannot be declared as constant
2. Non-`void` functions always have to return something
3. Functions objects cannot be available globally
4. A function can modify global variables

## 3. Keywords

### Return
Return returns a value to the calling point.

Example:
```cpp
int add(int a, int b){
    return a + b;
}

print(add(2,2) , "\n");
// And return gives
print(2 + 2 -> 4, "\n");
```

Return can only return values with the functions data type.