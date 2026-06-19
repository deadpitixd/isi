# ILS: Classes
Version: 1.0<br>
Status: Draft

Classes are a collection of objects packed into one object.

# 1. Basic Explanation
Classes can store objects in them, the objects are defined on compile time,
a class can also store functions.

Accessing member is done by adding a dot after writing the Classes identifier,
then you specify which member you want to address.

Classes are objects.

Classes can have interpreter overloads, like `.toString()`.

Classes also support encapsulation and inheritance, so a class can inherit from another class.

Example:
```cpp
class a{
    public {
        int a;
        int b;
        virtual void doSomething(){
            print(a + b);
        }
    }
};
class b : a {
    public {

        // Automatically inherits variables like [a, b]

        @Override;
        void doSomething(){
            print (a * b);
        }
    }
};
```


## 1.1 Example
```cpp
class myClass{
    // You can wrap elemnts in `public / private`
    public {
        int a;
        float b;
    }
    public float calculate(){
        return a + b;
    }
    // .toString interpreter overload
    // overloads are public by default
    overload string toString(){
        return f"{this.a}, {this.b}";
    }

    myClass(int x, float y) { a = x; b = y; }
};

// Usage:

// Constructor tells, that the first element is pointing to 'a',
// and the second one is pointing to 'b'.
myClass s{8, 12.4};

// You can access members by using the dot '.' operator.
print(s.calculate(),"\n");

// By default functions that accept strings, try to call .toString() if object isn't a string.
print(s,"\n"); // "8, 12.4"
```

## 1.2 `this` keyword
`this` allows the program to access an objects specific member, it can only be called from the object.

## 1.3 Optimizations
If the class only has itself in it, the class is invalid.

If the class only has one element, to save memory, all references of the class are to be replaced with the element.