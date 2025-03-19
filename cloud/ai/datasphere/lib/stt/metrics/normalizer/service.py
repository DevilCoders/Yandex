import grpc
import logging
import time
from concurrent import futures
from argparse import ArgumentParser

from .proto import normalizer_pb2
from .proto import normalizer_pb2_grpc

from cloud.ai.speechkit.stt.lib.eval.text.normalizer import Normalizer


class NormalizerServicer(normalizer_pb2_grpc.NormalizerServicer):
    def __init__(self, normalizer_data_path: str):
        self.normalizer = Normalizer(
            normalizer_data_path=normalizer_data_path,
            model='revnorm',
            language_code='ru-RU'
        )

    def NormalizeText(self, request, context):
        normalized_text = self.normalizer.transform(request.text)
        return normalizer_pb2.Response(text=normalized_text)


def serve(normalizer_data_path: str, max_workers: int):
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=max_workers))
    normalizer_pb2_grpc.add_NormalizerServicer_to_server(NormalizerServicer(normalizer_data_path), server)

    server.add_insecure_port('[::]:80')
    server.start()

    try:
        one_day_in_sec = 60 * 60 * 24
        while True:
            time.sleep(one_day_in_sec)
    except KeyboardInterrupt:
        server.stop(0)


def main():
    parser = ArgumentParser()
    parser.add_argument('--normalizer', type=str, required=True, help='normalizer data path')
    parser.add_argument('--max-workers', type=int, default=100, help='number of grpc threads')
    args = parser.parse_args()
    serve(args.normalizer, args.max_workers)


if __name__ == '__main__':
    logging.basicConfig()
    main()
