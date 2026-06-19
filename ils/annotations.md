# ILS: Annotations
Version: 1.0<br>
Status: Draft

Description of annotations that give certain properties to certain obejcts.

## 1. Basic description
Annotations are Java inspired keywords that give properties to objects,
they are defined with the at `(@)` character.

An annotation has to be placed before another annotation or an object that supports the annotation.

Basic annotation example:

```Java
@Nullable(0)
// (Objects are non-nullable by default)
void doSomething(int a){
    
}
```

## . Annotations

### @Nullable()
|   TYPE  |SUPPORT|
|:---------|-------:|
|Functions|   ✅ |
|Classes  |   ❌  |

---

Nullable takes in static positive integers as arguments, and tells the function, which elements are nullable.

> Function parameters are non-nullable by default

Nullable also supports range based ints, e.g
`(1 : 3)`

If nullable has no parameters, all function parameters are nullable.

Example:
```Java
@Nullable(1)
void doSomething(int a, int b, int c){
    ...
}
```

Here, only the first element is nullable.

### Override
|   TYPE  |SUPPORT|
|:---------|-------:|
|Functions|   *✅\** |
|Classes  |   ❌  |

*\* Only supported in classes*

---

Override overrides inherited public function in a class.

Override only overrides a function if in the inherited class the function is marked `virtual`, and if the functions parameters and name match.

Example:
```Java
class a{
    public {
        virtual void foo(int bar){

        }
    }
};
class b : a{
    public {
        @Override
        void foo(int bar){

        }
    }
}

```

> Functions with the same name without `@Override` won't override.