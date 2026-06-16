# ILS: Structs
Version: 1.0<br>
Status: Draft

Structs are a collection of objects packed into one object.

# 1. Basic Explanation
Structs can store objects in them, the objects are defined on compile time,
a struct can also store functions.

Accessing member is done by adding a dot after writing the structs identifier,
then you specify which member you want to address.

Structs are objects.

Structs can have interpreter overloads, like `.toString()`.

## 1.1 Example
```cpp
struct myStruct{
    int a=0;
    float b=0;
    float calculate(){
        return a + b;
    }
    // .toString interpreter overload
    overload string toString(){
        return f"{this.a}, {this.b}";
    }

    myStruct(int x, float y) { a = x; b = y; }
};

// Usage:

// Constructor tells, that the first element is pointing to 'a',
// and the second one is pointing to 'b'.
myStruct s{8, 12.4};

// You can access members by using the dot '.' operator.
print(s.calculate(),"\n");

// By default functions that accept strings, try to call .toString() if object isn't a string.
print(s,"\n"); // "8, 12.4"
```

## 1.2 `this` keyword
`this` allows the program to access an objects specific member, it can only be called from the object.

## 1.3 Optimizations
If the struct only has itself in it, the struct is invalid.

If the struct only has one element, to save memory, all references of the struct are to be replaced with the element.