import json
import logging
import os
import time

import numpy as np
import tensorflow as tf
import tornado.web
from tensorflow_serving.apis import predict_pb2
from tornado.gen import coroutine, Future
from tornado.ioloop import IOLoop

from ..app import config
from ..app.image import preprocess_image
from ..app.predictor import stub

logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger(__name__)


# https://github.com/grpc/grpc/wiki/Integration-with-tornado-(python)
def _fwrap(f, gf):
    try:
        f.set_result(gf.result())
    except Exception as e:
        f.set_exception(e)


def fwrap(gf):
    f = Future()

    gf.add_done_callback(
        lambda _: ioloop.add_callback(_fwrap, f, gf)
    )
    return f


class PingHandler(tornado.web.RequestHandler):
    def get(self):
        self.write('Pong')


class ClassifyHandler(tornado.web.RequestHandler):
    @coroutine
    def post(self):
        request_image = self.request.files['data'][0]
        model = self.request.arguments.get('model')[0]

        start_time = time.time()
        image = preprocess_image(request_image)
        image = tf.expand_dims(image, 0)  # tf-serving expects dims (1, 299, 299, 3)
        preprocess_image_time = time.time() - start_time

        start_time = time.time()

        request = predict_pb2.PredictRequest()
        request.model_spec.name = model
        request.model_spec.signature_name = 'predict'

        # TODO: delete if preprocess_image() will return numpy.array
        with tf.Session():
            image = image.eval()  # Tensor -> numpy array

        request.inputs['images'].CopyFrom(
            tf.contrib.util.make_tensor_proto(image, shape=image.shape))

        prediction = yield fwrap(stub.Predict.future(request, 10.0))

        prediction = np.array(prediction.outputs['scores'].float_val)

        prediction_time = time.time() - start_time

        index_to_class = config.MODEL_CLASS_MAP[model]
        prediction = {index_to_class[class_index]: class_prob for class_index, class_prob in enumerate(prediction)}

        logger.info(
            '{filename} prediction {classes}; '
            'preprocess image time: {preprocess_image_time}, prediction time: {prediction_time} [PID {pid}]'.format(
                filename=request_image.filename,
                classes=prediction,
                preprocess_image_time=preprocess_image_time,
                prediction_time=prediction_time,
                pid=os.getpid(),
            ))

        self.write(json.dumps({
            'classes': prediction,
            'model': str(model)
        }))


if __name__ == "__main__":
    app = tornado.web.Application([
        (r"/v1/classify", ClassifyHandler),
        (r"/ping", PingHandler),
    ])

    app.listen(config.rest_api_port)

    ioloop = IOLoop.current()
    ioloop.start()
