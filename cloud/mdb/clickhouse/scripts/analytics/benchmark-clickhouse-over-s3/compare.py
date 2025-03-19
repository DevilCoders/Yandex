import csv
import sys


def get_array(file):
    with open(file, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',', quotechar='|')
        array = [[float(element.strip()) for element in row] for row in reader]
        return array


a = get_array(sys.argv[1])
b = get_array(sys.argv[2])
difference = [[a - b for a, b in zip(arow, brow)] for arow, brow in zip(a, b)]
division = [[a / b for a, b in zip(arow, brow)] for arow, brow in zip(a, b)]


def print_array(array):
    for row in array:
        print(row)


print('difference:')
print_array(difference)
print('division:')
print_array(division)
