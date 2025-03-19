# Get graph.pb from https://sandbox.yandex-team.ru/resource/748707582/view

import os

import tensorflow as tf


def load_graph(frozen_graph_filename):
    print("Loading: ", frozen_graph_filename)
    with tf.gfile.GFile(frozen_graph_filename, "rb") as f:
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())

    with tf.Graph().as_default() as graph:
        tf.import_graph_def(graph_def, name="")
    return graph


model = load_graph("./graph.pb")

with tf.Session(graph=model) as sess:
    input_node = model.get_tensor_by_name("Input_NHWC:0")
    output_node = model.get_tensor_by_name("softmax:0")

    model_input = tf.saved_model.utils.build_tensor_info(input_node)
    model_output = tf.saved_model.utils.build_tensor_info(output_node)

    export_path_base = '../../models/badoo'
    export_version = 1

    export_path = os.path.join(
        tf.compat.as_bytes(export_path_base),
        tf.compat.as_bytes(str(export_version)))
    print('Exporting trained model to', export_path)
    builder = tf.saved_model.builder.SavedModelBuilder(export_path)

    prediction_signature = (
        tf.saved_model.signature_def_utils.build_signature_def(
            inputs={'images': model_input},
            outputs={'scores': model_output},
            method_name=tf.saved_model.signature_constants.PREDICT_METHOD_NAME))

    main_op = tf.group(tf.tables_initializer(), name='main_op')
    builder.add_meta_graph_and_variables(
        sess, [tf.saved_model.tag_constants.SERVING],
        signature_def_map={
            'predict':
                prediction_signature,
        },
        main_op=main_op)

    builder.save()
