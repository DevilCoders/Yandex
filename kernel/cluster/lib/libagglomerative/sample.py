import random
import math
import operator

import libagglomerative

x_means = range(0, 5)
y_means = range(0, 3)

random.shuffle(x_means)
random.shuffle(y_means)

sigma = 0.001

print x_means, y_means

data = []

for x in x_means:
    for y in y_means:
        count = int(random.random() * 3) + 3
        for i in range(count):
            sample_x = random.gauss(x, sigma)
            sample_y = random.gauss(y, sigma)

            print len(data), x, y, sample_x, sample_y

            data.append([sample_x, sample_y])

clustering = libagglomerative.AgglomerativeClustering(len(data))

def Similarity(a, b):
    x_diff = a[0] - b[0]
    y_diff = a[1] - b[1]

    return 0.1 / (x_diff * x_diff + y_diff * y_diff + 0.1)

for i in range(len(data)):
    for j in range(len(data)):
        print i, j, Similarity(data[i], data[j])
        clustering.Add(i, j, Similarity(data[i], data[j]))

print clustering.Build()
print clustering.SetupRelevances()
print clustering.Precision()
print clustering.Recall()
print '\n'.join(map(str, clustering.Clusters()))
