import yt.wrapper as yt
import os
import pymorphy2
import logging.config
import pickle
import pandas as pd
import numpy as np
import nirvana_dl
import tensorflow as tf
from tensorflow.keras.layers import Embedding, GlobalMaxPool1D, Dense
from tensorflow.keras import Sequential
from tensorflow.keras.optimizers import Adam
from sklearn.preprocessing import MultiLabelBinarizer
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from keras_preprocessing.text import Tokenizer
from keras_preprocessing.sequence import pad_sequences

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

output_path = os.environ['TMP_OUTPUT_PATH']
logger.debug(output_path)
yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
yt.config["token"] = os.environ['YT_TOKEN']

ma = pymorphy2.MorphAnalyzer()
maxlen=100


def add_common_components(predictions, common_components=['billing', 'mdb', 'compute']):
    for component in predictions:
        for common in common_components:
            if component.startswith(common) and component != common:
                predictions.append(common)
    return list(set(predictions))


def main():
    df_train = pd.DataFrame(yt.read_table(nirvana_dl.params()["train_table"]))[['key', 'clean_text', 'real_components']]
    df_val = pd.DataFrame(yt.read_table(nirvana_dl.params()["val_table"]))[['key', 'clean_text', 'real_components']]
    components_white_list = list(pd.DataFrame(yt.read_table(nirvana_dl.params()["components_white_list"]))['component_names'].values)

    logger.debug("Start Preprocessing")

    def clean_compomemts(componet_list):
        component_encoder = dict(zip(components_white_list, range(len(components_white_list))))
        componet_list = list(set(componet_list) & set(components_white_list))
        return list(map(lambda s: component_encoder[s], add_common_components(componet_list)))

    tokenizer = Tokenizer(num_words=5000, lower=True)
    tokenizer.fit_on_texts(df_train['clean_text'].values)
    X_train = pad_sequences(tokenizer.texts_to_sequences(df_train['clean_text'].values), maxlen=maxlen)
    X_val = pad_sequences(tokenizer.texts_to_sequences(df_val['clean_text'].values), maxlen=maxlen)

    mlb = MultiLabelBinarizer()
    temp = list(df_train['real_components'].apply(clean_compomemts).values)
    temp.append(list(range(len(components_white_list))))
    y_train = mlb.fit_transform(temp)[:-1, :]
    y_val = mlb.transform(df_val['real_components'].apply(clean_compomemts).values)

    logger.debug('Started Training')
    model = Sequential()
    model.add(Embedding(5000, 64, input_length=maxlen))
    model.add(GlobalMaxPool1D())
    model.add(Dense(y_train.shape[1], activation='sigmoid'))
    model.compile(optimizer=Adam(0.015), loss='binary_crossentropy', metrics=[tf.keras.metrics.AUC()])

    callback = tf.keras.callbacks.EarlyStopping(
        patience=3,
        monitor='val_loss',
        mode='auto'
    )

    model.fit(
        X_train,
        y_train,
        validation_data=(X_val, y_val),
        callbacks=[callback],
        batch_size=128,
        epochs=10
    )
    logger.debug('Training finished')

    yt_adapter = YTAdapter()
    df_val['predicted_components'] = [list(np.array(components_white_list)[x > 0.37]) for x in model.predict(X_val)]
    df_train['predicted_components'] = [list(np.array(components_white_list)[x > 0.37]) for x in model.predict(X_train)]
    logger.debug(df_val.columns)
    schema ={
        "key": "string",
        "real_components": "any",
        "predicted_components": "any"}
    yt_adapter.save_result(
        result_path="//home/cloud_analytics/ml/support_tickets_classification/Val",
        schema=schema,
        df=df_val.drop('clean_text', axis=1),
        append=False)
    yt_adapter.save_result(
        result_path="//home/cloud_analytics/ml/support_tickets_classification/Train",
        schema=schema,
        df=df_train.drop('clean_text', axis=1),
        append=False)

    logger.debug('Saving results...')
    os.mkdir(f'{output_path}/model')
    model.save(f'{output_path}/model/model')
    with open(f'{output_path}/model/tokenizer.pkl', 'wb+') as handle:
        pickle.dump(tokenizer, handle, protocol=pickle.HIGHEST_PROTOCOL)

if __name__ == '__main__':
    main()
