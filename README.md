# Pasathai

A Thai-language interpreter written in C with mark-and-sweep garbage collection.

## Usage

```sh
make                        # build
./pasathai somefile.thai   # run file
./pasathai                 # interactive REPL
```

## Syntax

```thai
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

# Conditionals
ถ้า (x > 5) {
    แสดง("greater");
} อื่นๆ {
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