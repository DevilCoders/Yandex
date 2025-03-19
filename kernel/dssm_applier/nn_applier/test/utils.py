import subprocess
import os

def lines_equal(line1, line2):
    items1 = [float(item) for item in line1.split()]
    items2 = [float(item) for item in line2.split()]
    if len(items1) != len(items2):
        return False
    for v1, v2 in zip(items1, items2):
        if abs(v1 - v2) / (abs(v1) + abs(v2)) > 0.001 and abs(v1 - v2) > 0.001:
            return False
        return True
