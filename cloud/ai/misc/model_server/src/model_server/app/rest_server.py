import logging
import os
import time

import numpy as np
import tensorflow as tf
from flask import Flask
from flask import jsonify
from flask import request

import config
from image import preprocess_image
from predictor import get_prediction

app = Flask(__name__)

logging.basicConfig(level=logging.DEBUG)


class InvalidUsage(Exception):
    status_code = 400

    def __init__(self, message, status_code=None):
        Exception.__init__(self)
        self.message = message
        if status_code is not None:
            self.status_code = status_code

    def to_dict(self):
        return {'message': self.message}


@app.errorhandler(InvalidUsage)
def handle_invalid_usage(error):
    response = jsonify(error.to_dict())
    response.status_code = error.status_code
    return response


@app.route('/v1/classify', methods=['POST'])
def predict():
    if not request.files.get('data') or not request.values.get('model'):
        raise InvalidUsage('Invalid data format')

    request_image = request.files['data']
    model = request.values.get('model')

    image_filename = request_image.filename
    if not image_filename.endswith('.jpg') and not image_filename.endswith('.jpeg'):
        raise InvalidUsage('Only JPEG images are supported')

    start_time = time.time()
    preprocessed_image = preprocess_image(request_image)
    preprocessed_image = tf.expand_dims(preprocessed_image, 0)  # tf-serving expects dims (1, 299, 299, 3)
    preprocess_image_time = time.time() - start_time

    start_time = time.time()
    # TODO: delete if preprocess_image() will return numpy.array
    with tf.Session():
        preprocessed_image = preprocessed_image.eval()  # Tensor -> numpy array
    prediction = get_prediction(preprocessed_image, model)
    prediction_time = time.time() - start_time

    app.logger.debug(
        '{filename} prediction {classes}; '
        'preprocess image time: {preprocess_image_time}, '
        'prediction time: {prediction_time} [PID {pid}]'.format(
            filename=image_filename,
            classes=prediction,
            preprocess_image_time=preprocess_image_time,
            prediction_time=prediction_time,
            pid=os.getpid(),
        ))

    response = jsonify({
        'classes': prediction,
        'model': model
    })
    return response


def tf_serving_warmup():
    random_tensor = np.float32(np.random.random((1, 299, 299, 3)))
    get_prediction(random_tensor, 'badoo')


tf_serving_warmup()

if __name__ == "__main__":
    app.logger.info('Listening on %d' % config.rest_api_port)
    app.run(host='::', port=config.rest_api_port)
    app.debug = False
