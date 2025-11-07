# Pasathai

A Thai-language interpreter written in C with mark-and-sweep garbage collection.

## Usage

```sh
make                        # build
./pasathai somefile.thai   # run file
./pasathai                 # interactive REPL
```

## Examples

See the `/examples` 

## Supported Feature

- Variables and assignment
- Arithmetic operations (`+`, `-`, `*`, `/`, `%`)
- String concatenation
- Boolean values and null
- **Arrays** with indexing and nested arrays
- **Array built-ins**: `len()`, `push()`, `pop()`
- **Comments**: `#` single-line comments
- Conditionals (`ถ้า`, `ไม่งั้น`)
- **For loops** (`สำหรับ ... จาก ... ถึง/ก่อนถึง`)
- While loops (`ขณะที่`)
- Functions and closures (`ฟังก์ชัน`)
- Mark-and-sweep garbage collection
- Error messages with source location

## Syntax

```
# Variables
ให้ x = 10;
ให้ name = "สวัสดี";

# Arithmetic & Modulo
ให้ sum = 5 + 3;
ให้ remainder = 17 % 5;  # 2

# Strings
ให้ greeting = "Hello" + " " + "World";
แสดง(greeting);  # prints: Hello World

# Booleans & Null
ให้ flag = จริง;        # true
ให้ empty = ว่างเปล่า;  # null

# Arrays
ให้ numbers = [1, 2, 3, 4, 5];
แสดง(numbers[0]);  # 1
แสดง(numbers[2]);  # 3

# Nested Arrays
ให้ matrix = [[1, 2], [3, 4]];
แสดง(matrix[0]);     # [1, 2]
แสดง(matrix[0][1]);  # 2

# Empty Arrays
ให้ empty_list = [];

# Array Built-in Functions
ให้ arr = [1, 2, 3];
แสดง(len(arr));      # 3 - get array length
push(arr, 4);        # add element to end
แสดง(arr);           # [1, 2, 3, 4]
ให้ last = pop(arr); # remove and return last element
แสดง(last);          # 4

# len() also works with strings
ให้ text = "Hello";
แสดง(len(text));     # 5

# For Loops
# Counted loop (inclusive end)
สำหรับ i จาก 0 ถึง 9 {
    แสดง(i);  # prints 0, 1, 2, ..., 9
}

# Counted loop (exclusive end)
สำหรับ i จาก 0 ก่อนถึง 10 {
    แสดง(i);  # prints 0, 1, 2, ..., 9
}

# Using for loops with arrays
ให้ arr = [10, 20, 30, 40];
สำหรับ i จาก 0 ก่อนถึง len(arr) {
    แสดง(arr[i]);
}

# Building arrays with for loops
ให้ squares = [];
สำหรับ n จาก 1 ถึง 5 {
    push(squares, n * n);
}
แสดง(squares);  # [1, 4, 9, 16, 25]

# Conditionals
ถ้า (x > 5) {
    แสดง("greater");
} ไม่งั้น {
    แสดง("less or equal");
}

# While Loops
ให้ i = 0;
ขณะที่ (i < 5) {
    แสดง(i);
    ให้ i = i + 1;
}

# Functions
ให้ add = ฟังก์ชัน(a, b) {
    คืนค่า a + b;
};
แสดง(add(10, 20));  # 30

# Closures
ให้ makeCounter = ฟังก์ชัน() {
    ให้ count = 0;
    คืนค่า ฟังก์ชัน() {
        ให้ count = count + 1;
        คืนค่า count;
    };
};
```