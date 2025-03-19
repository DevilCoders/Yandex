import argparse
from concurrent import futures

import grpc

from cloud.gauthling.gauthling_daemon.proto import gauthling_pb2_grpc
from gauthling_daemon_mock import ControlServer, GauthlingMockServicer, AccessServiceMockServicer
from yandex.cloud.priv.servicecontrol.v1 import access_service_pb2_grpc


def parse_args():
    parser = argparse.ArgumentParser(description="Gauthling Mock Server")
    parser.add_argument("--host", dest="host", default="0.0.0.0",
                        help="Host to listen on")
    parser.add_argument("--max-workers", dest="max_workers", type=int, default=1,
                        help="The maximum number of threads that can be used to execute the given calls")
    parser.add_argument("--port", dest="port", type=int, default=4284,
                        help="Mock server port")
    parser.add_argument("--control-server-port", dest="control_server_port", type=int, default=2484,
                        help="Control server port")
    return parser.parse_args()


def run(host="0.0.0.0", port=4284, control_server_port=2484, max_workers=1):
    gauthling = GauthlingMockServicer()
    access_service = AccessServiceMockServicer()
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=max_workers))
    gauthling_pb2_grpc.add_GauthlingServiceServicer_to_server(gauthling, server)
    access_service_pb2_grpc.add_AccessServiceServicer_to_server(access_service, server)
    server.add_insecure_port("{0}:{1}".format(host, port))
    server.start()

    control_server = ControlServer(gauthling, access_service, "GauthlingMockControl")
    control_server.run(host=host, port=control_server_port, threaded=True)


if __name__ == "__main__":
    args = parse_args()
    run(
        host=args.host,
        port=args.port,
        control_server_port=args.control_server_port,
        max_workers=args.max_workers,
    )
