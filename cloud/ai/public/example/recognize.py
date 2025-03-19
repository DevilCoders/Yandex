from argparse import ArgumentParser

import requests

from speechkit import model_repository, configure_credentials
from speechkit.common import Product
from speechkit.stt import RecognitionConfig, AudioProcessingType

configure_credentials(
    yc_ai_token='Api-Key ...',
    azure_token='...',
    azure_region='...'
)


def recognize(audio, product, mode, language,
              host, ssl, x_service_branch, x_node_alias, x_node_id):
    assert sum([x_service_branch is not None, x_node_alias is not None, x_node_id is not None]) == 1

    service_branch = None
    if x_service_branch is not None:
        service_branch = ('x-service-branch', x_service_branch)
    if x_node_alias is not None:
        service_branch = ('x-node-alias', x_node_alias)
    if x_node_id is not None:
        service_branch = ('x-node-id', x_node_id)

    endpoint = None
    if host is not None:
        endpoint = (host, ssl)

    model = model_repository.recognition_model(
        product={'yandex': Product.Yandex, 'azure': Product.Azure}[product],
        service_branch=service_branch,
        custom_endpoint=endpoint
    )

    config = RecognitionConfig(
        mode={'full': AudioProcessingType.Full, 'stream': AudioProcessingType.Stream}[mode],
        language=language
    )
    
    result = model.transcribe_file(audio, config)
    for c, res in enumerate(result):
        print(f'channel: {c}\nraw_text: {res.raw_text}\nnorm_text: {res.normalized_text}\n')


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--audio', type=str, help='audio path', required=True)
    parser.add_argument('--product', choices=['yandex', 'azure'], help='product name', default='yandex')
    parser.add_argument('--mode', choices=['full', 'stream'], help='mode', default='full')
    parser.add_argument('--language', type=str, help='language', default='ru-RU')
    parser.add_argument('--host', type=str, help='host path', default=None)
    parser.add_argument('--ssl', action='store_true', help='use ssl', default=False)
    parser.add_argument('--node-alias', type=str, help='datasphere alias', default=None)
    parser.add_argument('--service-branch', type=str, help='service branch', default=None)
    parser.add_argument('--node-id', type=str, help='datasphere node id', default=None)

    args = parser.parse_args()

    recognize(args.audio, args.product, args.mode, args.language,
              args.host, args.ssl, args.service_branch, args.node_alias, args.node_id)
