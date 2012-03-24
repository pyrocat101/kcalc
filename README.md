# KCalc: simple kernel calculator module

## Usage

    echo "1 + 1" > /dev/kcalc
    cat /dev/kcalc

`write` the pseudo /dev/kcalc device with the expression, then you can get result simply with counterpart system call `read`.

## TODO

* Read/Write lock
* Permission FIX
* Blocked read
* Larger tests
