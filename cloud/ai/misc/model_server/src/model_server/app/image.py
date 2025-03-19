import tensorflow as tf


# Implements image preprocessing for Inception networks.

def preprocess_image(image):
    image_tensor = tf.image.decode_jpeg(image.read(), channels=3)
    return preprocess_image_tensor(image_tensor, 299, 299)


# https://github.com/tensorflow/models/blob/b2522f9bb9a559e148502a67ebd9a6592320cb0f/research/slim/preprocessing/inception_preprocessing.py#L244
def preprocess_image_tensor(image, height, width, central_fraction=0.875):
    if image.dtype != tf.float32:
        image = tf.image.convert_image_dtype(image, dtype=tf.float32)
    if central_fraction:
        image = tf.image.central_crop(image, central_fraction=central_fraction)

    if height and width:
        image = tf.expand_dims(image, 0)
        image = tf.image.resize_bilinear(image, [height, width],
                                         align_corners=False)
        image = tf.squeeze(image, [0])
    image = tf.subtract(image, 0.5)
    image = tf.multiply(image, 2.0)
    return image
