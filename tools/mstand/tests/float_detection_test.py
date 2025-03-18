#!/usr/bin/env python3

import re
import time


def could_be_float1(str_number):
    c = str_number[0]
    if not (c.isdigit() or c == "-" or c == "."):
        return False
    return True


def could_be_float2(str_number):
    if str_number[0] not in "1234567890-.":
        return False
    return True


def could_be_float3(str_number):
    try:
        float(str_number)
    except ValueError:
        return False
    return True


ord_0 = ord('0')
ord_9 = ord('9')
ord_dot = ord('.')
ord_minus = ord('-')


def could_be_float4(str_number):
    c = ord(str_number[0])
    if not (ord_0 <= c <= ord_9 or c == ord_dot or c == ord_minus):
        return False
    return True


float_set = set("1234567890-.")


def could_be_float5(str_number):
    if str_number[0] not in float_set:
        return False
    return True


float_re = re.compile(r"^[0-9.-].*")


def could_be_float6(str_number):
    if not float_re.match(str_number):
        return False
    return True


def read_it(func):
    start = time.time()
    print("started with ", func)
    num = 0
    not_num = 0
    with open("numbers.txt") as fd:
        for line in fd:
            if func(line):
                num += 1
            else:
                not_num += 1
    print(num, not_num)
    print(func, " ended in ", time.time() - start)


def main():
    print("generating")
    with open("numbers.txt", "w") as fd:
        for x in range(50000000):
            if x % 17 != 0:
                fd.write("{}".format(float(x) / 3.0))
            else:
                fd.write("[{}]".format(x))
            fd.write("\n")

    print("generating done")

    read_it(could_be_float1)
    read_it(could_be_float2)
    read_it(could_be_float3)
    read_it(could_be_float4)
    read_it(could_be_float5)
    read_it(could_be_float6)

if __name__ == "__main__":
    main()
