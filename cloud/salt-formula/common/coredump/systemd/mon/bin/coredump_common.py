#!/usr/bin/env python3
import coredump

def main():
    name = "coredump_common"
    mask = coredump.Mask()
    print(coredump.check(name, mask))

if __name__ == "__main__":
    main()
