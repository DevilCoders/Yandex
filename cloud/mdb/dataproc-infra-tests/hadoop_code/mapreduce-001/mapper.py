#!/usr/bin/python
import sys

population = sum(int(line.split('\t')[14]) for line in sys.stdin)
print(population)
