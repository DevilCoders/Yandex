import pandas as pd
import logging.config
import logging
import pkg_resources
import pickle
import click
import time
import threading
import yt.wrapper as yt
import zipfile
import os

from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter
from clan_tools.secrets.Vault import Vault
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config

from tensorflow import keras
from keras_preprocessing.sequence import pad_sequences

from collections import defaultdict
from support_tickets_classification.preprocessing import clean_text
from flask import request
from flask import Flask


default_log_config['handlers']['file']['filename'] = '/tmp/support_tickets_classification.log'
path_to_models_yt = "//home/cloud_analytics/ml/support_tickets_classification/models"
server_model_path = "/usr/local/lib/python3.7/dist-packages/support_tickets_classification/model"

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


app = Flask(__name__)

keywords = None

Vault().get_secrets()
tracker_adapter = TrackerAdapter()
tracker_client = tracker_adapter.st_client
yt.config["proxy"]["url"] = "hahn"
yt.config["token"] = os.environ['YT_TOKEN']


def get_keywords() -> pd.DataFrame:
    global keywords
    if keywords is None:
        keywords_path = pkg_resources.resource_filename(
            'support_tickets_classification.data', "keywords.csv")
        keywords = pd.read_csv(keywords_path, skiprows=[1])
    return keywords


def granular_components(predictions, common_components=['billing', 'mdb', 'compute']):
    data = defaultdict(int)
    for component in predictions:
        for common in common_components:
            if component.startswith(common):
                data[common] +=1
    for common in common_components:
        if data[common] >= 2:
            try:
                predictions.remove(common)
            except ValueError:
                pass
    return predictions


def check_billing(id, text):
    attachment_list = tracker_client.issues[id].attachments.get_all()
    if len(attachment_list) != 1:
        return False
    if not attachment_list[0].name.endswith('.pdf'):
        return False
    full_text = text + " " + clean_text(attachment_list[0].name)
    for word in ["платёжный", "поручение", "report", "счет", "invoice", "баланс"]:
        if word in full_text:
            return True
    return False


@timing
def classify(
        text: str,
        threshold: float = 0.35,
        trained_model_path: str = pkg_resources.resource_filename('support_tickets_classification.data.model', "model"),
        tokenizer_path: str = pkg_resources.resource_filename('support_tickets_classification.data.model', 'tokenizer.pkl'),
        components_white_list_path: str = pkg_resources.resource_filename('support_tickets_classification.data.model', 'components_white_list.csv')):
    
    cleaned_text = clean_text(text)
    components_white_list = pd.read_csv(components_white_list_path)['component_names'].values
    with open(tokenizer_path, 'rb') as handle:
        tokenizer = pickle.load(handle)
    sequences = tokenizer.texts_to_sequences([cleaned_text])
    X = pad_sequences(sequences, maxlen=100)

    model = keras.models.load_model(trained_model_path)

    y_pred = model.predict(X) > threshold  # bigger the threshold -> lesser the precision
    return list(components_white_list[y_pred[0]])


@app.route('/', methods=['POST', 'GET'])
@timing
def classify_endpoint(num_attepts: int = 3):
    if request.method == 'POST':
        ticket = request.json['ticket']
        logger.debug(f'Classifying issue {ticket}')
        issue = tracker_client.issues[ticket]

        text = issue.summary + ' ' + issue.description

        classified_components = list(set(classify(text)))
        classified_components = granular_components(classified_components)
        classified_components_str = ', '.join(classified_components)
        if len(classified_components_str) == 0:
            classified_components_str = '-'
        support_components = list(map(lambda x: x.as_dict()['display'], issue.components))
        logger.debug(f'{ticket} was classified as: {classified_components_str}')
        logger.debug(f'{ticket} support components: {support_components}')
        attempts = 0
        while attempts < num_attepts:
            try:
                if (classified_components_str == '-') or any([comp in support_components for comp in ['tracker', 'wiki', 'forms', 'квоты']]):
                    issue.update(additionalTags={'set': classified_components_str})
                elif 'billing' in classified_components_str:
                    if check_billing(ticket, text):
                        issue.update(additionalTags={'set': classified_components_str}, components={'set': ['billing.wire_transfers']})
                    else:
                        issue.update(additionalTags={'set': classified_components_str}, components={'set': classified_components})
                else:
                    issue.update(additionalTags={'set': classified_components_str}, components={'set': classified_components})
                break
            except Exception as e:
                attempts += 1
                logger.debug(f'Attempt {attempts} failed with {e}')
                time.sleep(3)

        return {'components': classified_components, 'ticket': ticket}

    return 'Request should be POST'


@click.command()
@click.option('--port', type=click.INT, default=5000)
@click.option('--host', default='::')
def main(port: int, host: str):
    app.run(host=host, port=port)


def download_model():
    model_last_version = max(yt.list(path_to_models_yt))
    file = yt.read_file(f'{path_to_models_yt}/{model_last_version}')

    binary_file_path = (f"{server_model_path}/model.zip")
    with open(binary_file_path, 'wb') as f:
        f.write(file.read())

    with zipfile.ZipFile(f"{server_model_path}/model.zip", "r") as zip_ref:
        zip_ref.extractall(f"{server_model_path}/model_tmp")
    logger.debug(f'Downloaded model version {model_last_version}')
    os.rename(f"{server_model_path}/model_tmp", f"{server_model_path}/model")


def update_model():
    while True:
        download_model()
        time.sleep(86400)


if __name__ == "__main__":
    download_model()
    model_updating_thread = threading.Thread(target=update_model)
    model_updating_thread.start()
    main()
