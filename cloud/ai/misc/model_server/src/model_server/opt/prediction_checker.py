# coding: utf-8

# Comparing results from TF graph and from container. Before running start container with "make run".
#
# model-server$ PYTHONPATH=src/ ~/venv/bin/python -m model_server.opt.prediction_checker

import argparse
import os
import sys

import requests
import tensorflow as tf

from ..app.config import MODEL_CLASS_MAP
from ..app.image import preprocess_image


def load_graph(frozen_graph_filename):
    print("Loading: ", frozen_graph_filename)
    with tf.gfile.GFile(frozen_graph_filename, "rb") as f:
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())

    with tf.Graph().as_default() as graph:
        tf.import_graph_def(graph_def, name="")
    return graph


parser = argparse.ArgumentParser()
parser.add_argument(
    '--model-name',
    dest='model_name',
    default='badoo',
    help='model name'
)
parser.add_argument(
    '--model-path',
    dest='model_path',
    default='misc/badoo/graph.pb',
    help='path to model'
)
parser.add_argument(
    '--images-dir',
    dest='images_dir',
    default='images/',
    help='path to images directory'
)
context = parser.parse_args(sys.argv[1:])

graph = load_graph(context.model_path)

input_node = graph.get_tensor_by_name("Input_NHWC:0")
output_node = graph.get_tensor_by_name("softmax:0")

model = context.model_name
index_to_class = MODEL_CLASS_MAP[model]

sess = tf.Session(graph=graph)


def get_raw_tf_predictions(image_tensor):
    class_index_probs = sess.run([output_node], feed_dict={input_node: [image_tensor]})[0][0]
    return {index_to_class[class_index]: class_prob
            for class_index, class_prob in enumerate(class_index_probs)}


def get_tf_serving_predictions(image_file):
    return requests.post(
        'http://127.0.0.1/v1/classify',
        files={'data': image_file},
        data={'model': 'badoo'}
    ).json()['classes']


problem_image_classes = []

for filename in os.listdir(context.images_dir):
    if not filename.endswith('jpg'):
        continue

    print('Image %s' % filename)

    with open(context.images_dir + '/' + filename, 'rb') as image_file:
        tf_serving_classes = get_tf_serving_predictions(image_file)

    with open(context.images_dir + '/' + filename, 'rb') as image_file:
        image = preprocess_image(image_file)
        with tf.Session():
            image = image.eval()
        raw_tf_classes = get_raw_tf_predictions(image)

    for class_name in index_to_class.values():
        tf_serving_prob = tf_serving_classes[class_name]
        raw_tf_prob = raw_tf_classes[class_name]

        if abs(tf_serving_prob - raw_tf_prob) > 1e-7:
            problem_image_classes.append((filename, class_name))

        print(class_name, tf_serving_prob, raw_tf_prob)

    print('')

if len(problem_image_classes):
    print('Problem image classes:')
    print problem_image_classes
else:
    print('No problems')
