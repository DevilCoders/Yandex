import sys
from typing import Union, Optional, Tuple

from speechkit.common import Product, ServiceBranch, Language
from speechkit.stt import AudioProcessingType, RecognitionModel, RecognizerFabric
from speechkit.tts import SynthesisModel


def _get_tts_endpoint() -> Tuple[str, bool]:
    return 'tts.api.cloud.yandex.net:443', True


def _tts_service_branch_header(endpoint: str,
                               branch: ServiceBranch) -> Tuple[str, str]:
    if 'tts.api' in endpoint:
        branch_key = 'x-service-branch'
    else:
        branch_key = 'x-node-alias'

    return branch_key, f'{branch}'


def _get_stt_endpoint() -> Tuple[str, bool]:
    return 'stt.api.cloud.yandex.net:443', True


def _stt_service_branch_header(branch: ServiceBranch) -> Tuple[str, str]:
    return 'x-node-alias', f'speechkit.stt.{branch}'


def synthesis_model(voice: str = None,
                    service_branch: ServiceBranch = ServiceBranch.Stable,
                    custom_endpoint: Tuple[str, bool] = None) -> SynthesisModel:
    endpoint, ssl = _get_tts_endpoint()
    if custom_endpoint is not None:
        endpoint, ssl = custom_endpoint

    branch_key, branch_value = _tts_service_branch_header(endpoint,
                                                          service_branch)
    return SynthesisModel(endpoint=endpoint,
                          use_ssl=ssl,
                          voice=voice,
                          branch_key_value=(branch_key, branch_value))


def recognition_model(product: Product = Product.Yandex,
                      service_branch: Optional[Union[ServiceBranch, Tuple[str, str]]] = None,
                      custom_endpoint: Optional[Tuple[str, bool]] = None) -> RecognitionModel:
    if product == Product.Yandex:
        endpoint, ssl = _get_stt_endpoint()
        if custom_endpoint is not None:
            endpoint, ssl = custom_endpoint
        if service_branch is not None and isinstance(service_branch, ServiceBranch):
            service_branch = _stt_service_branch_header(service_branch)

        model = RecognizerFabric().create(product, endpoint=endpoint, use_ssl=ssl, service_branch=service_branch)
    else:
        if custom_endpoint is not None:
            print(f'Ignore custom endpoint for {product.value} model', file=sys.stderr)
        if service_branch is not None:
            print(f'Ignore service branch for {product.value} model', file=sys.stderr)
        model = RecognizerFabric().create(product)

    return model
