import argparse
import sys

parser = argparse.ArgumentParser(description='select good offset for classifier model')
parser.add_argument('--part', help='part of input to be selected with classifier')
args = parser.parse_args()

predictions = []
for line in sys.stdin:
    prediction = float(line.split('\t')[1].strip())
    predictions.append(prediction)
predictions.sort(reverse = True)

for i in range(1, 100):
    print >> sys.stderr, i, predictions[i * len(predictions) / 100]

print -predictions[int(float(args.part) * len(predictions))]
