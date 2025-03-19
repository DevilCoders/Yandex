import numpy as np
import tensorflow as tf
from grpc.beta import implementations
from tensorflow_serving.apis import predict_pb2
from tensorflow_serving.apis import prediction_service_pb2

import config

channel = implementations.insecure_channel('127.0.0.1', 9001)
stub = prediction_service_pb2.beta_create_PredictionService_stub(channel)


def get_prediction(image, model):
    request = predict_pb2.PredictRequest()
    request.model_spec.name = model
    request.model_spec.signature_name = 'predict'

    request.inputs['images'].CopyFrom(
        tf.contrib.util.make_tensor_proto(image, shape=image.shape))

    result = stub.Predict(request, 10.0)
    prediction = np.array(result.outputs['scores'].float_val)

    index_to_class = config.MODEL_CLASS_MAP[model]

    return {
        index_to_class[class_index]: class_prob
        for class_index, class_prob in enumerate(prediction)
    }
