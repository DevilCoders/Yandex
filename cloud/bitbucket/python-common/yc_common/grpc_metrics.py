import time
import grpc

from yc_common.metrics import Metric, MetricTypes, time_buckets_ms

UNARY = "unary"

CLIENT_STARTED_COUNTER = Metric(
    MetricTypes.COUNTER, name="grpc_client_started_total",
    label_names=["grpc_type", "grpc_service", "grpc_method"],
    doc="Total number of RPCs started on the client.")

CLIENT_HANDLED_COUNTER = Metric(
    MetricTypes.COUNTER, name="grpc_client_handled_total",
    label_names=["grpc_code", "grpc_type", "grpc_service", "grpc_method"],
    doc="Total number of RPCs completed by the client, regardless of success or failure.")

## Metrics for the future use
#CLIENT_STREAM_MSG_RECEIVED = Metric(
#    MetricTypes.COUNTER, name="grpc_client_msg_received_total",
#    label_names=["grpc_type", "grpc_service", "grpc_method"],
#    doc="Total number of RPC stream messages received by the client.")
#
#CLIENT_STREAM_MSG_SENT = Metric(
#    MetricTypes.COUNTER, name="grpc_client_msg_sent_total",
#    label_names=["grpc_type", "grpc_service", "grpc_method"],
#    doc="Total number of gRPC stream messages sent by the client.")

CLIENT_HANDLED_HISTOGRAM = Metric(
    MetricTypes.HISTOGRAM, name="grpc_client_handling_seconds",
    label_names=["grpc_type", "grpc_service", "grpc_method"],
    doc="Histogram of response latency (seconds) of the gRPC until it is finished by the application.",
    buckets=time_buckets_ms)


def split_method_name(full_method_name):
    if full_method_name.startswith("/"):
        full_method_name = full_method_name.lstrip("/")
    parts = full_method_name.split("/", maxsplit=2)
    if len(parts) == 2:
        return parts
    else:
        return "unknown", "unknown"


class MetricClientInterceptor(grpc.UnaryUnaryClientInterceptor):

    @staticmethod
    def _callback(started_at, call_params):
        def callback(future_response):
            if type(future_response) == grpc.RpcError:
                code = grpc.StatusCode.UNKNOWN
            else:
                code = grpc.StatusCode.OK
                exception = future_response.exception()
                if exception is not None:
                    if isinstance(exception, grpc.Call):
                        code = exception.code()
                    else:
                        code = grpc.StatusCode.UNKNOWN

            CLIENT_HANDLED_COUNTER.labels(code.name, *call_params).inc()
            CLIENT_HANDLED_HISTOGRAM.labels(*call_params).observe((time.monotonic() - started_at) * 1000)

        return callback

    def intercept_unary_unary(self, continuation, client_call_details, request):
        call_params = (UNARY, *split_method_name(client_call_details.method))

        started_at = time.monotonic()
        CLIENT_STARTED_COUNTER.labels(*call_params).inc()

        response = continuation(client_call_details, request)
        response.add_done_callback(self._callback(started_at, call_params))
        return response
