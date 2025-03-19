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


def recognize(voice, text, export_path, host, ssl, node_alias, node_id):
    assert not all([node_alias is not None, node_id is not None])

    service_branch = None
    if node_alias is not None:
        service_branch = ('x-node-alias', node_alias)
    if node_id is not None:
        service_branch = ('x-node-id', node_id)

    endpoint = None
    if host is not None:
        endpoint = (host, ssl)

    model = model_repository.synthesis_model(
        voice=voice,
        service_branch=service_branch,
        custom_endpoint=endpoint
    )

    result = model.synthesize(text)
    result.export(export_path, format='wav')


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--voice', type=str, help='voice identifier', required=True)
    parser.add_argument('--text', type=str, help='text to synthesize', required=True)
    parser.add_argument('--export', type=str, help='path to export audio', required=False)
    parser.add_argument('--host', type=str, help='host path', default=None)
    parser.add_argument('--ssl', action='store_true', help='use ssl', default=False)
    parser.add_argument('--node-alias', type=str, help='datasphere alias', default=None)
    parser.add_argument('--node-id', type=str, help='datasphere node id', default=None)

    args = parser.parse_args()

    recognize(args.voice, args.text, args.export, args.host, args.ssl, args.node_alias, args.node_id)
