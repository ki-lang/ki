
# Types

## Names

```
ptr
class
funcref
string
byte (unsigned char, ui8)
i8
ui8
i16
ui16
int (ssize_t, i32 or i64)
uint (size_t, ui32 or ui64)
i32
i64
ui32
ui64
float (f32 or f64)
f32 (float)
f64 (double)
```

## Allowed assignments

```
ptr = ptr
ptr = class
classA = classA
number = number
```

## Extra assignments

```
makeptr {name} from size_t-value // make pointer from number
setptrv {var} to number-value // set pointer value to a number
getptrv {var} as {any-type} // get pointer value as a type
cast {var} as {type-of-same-bytes} // convert any-var to any type of same bytes 
```

