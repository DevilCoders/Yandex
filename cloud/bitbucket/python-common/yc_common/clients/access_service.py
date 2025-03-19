import grpc
import threading

import yc_as_client

from yc_common import config
from yc_common import misc
from yc_common import models


DEFAULT_CA_PATH = "/etc/ssl/certs/ca-certificates.crt"
DEFAULT_TIMEOUT = 2
DEFAULT_RETRIES = 7
DEFAULT_KEEPALIVE_TIME = 10 * 1000  # ms
DEFAULT_KEEPALIVE_TIMEOUT = 1 * 1000  # ms


class AccessService(yc_as_client.YCAccessServiceClient):
    __singleton_lock = threading.Lock()
    __singleton_instance = None

    @classmethod
    def instance(cls):
        disabled = not config.get_value("endpoints.access_service.enabled", default=False)
        if disabled:
            return None

        if not cls.__singleton_instance:
            with cls.__singleton_lock:
                if not cls.__singleton_instance:
                    url = config.get_value("endpoints.access_service.url")
                    tls = config.get_value("endpoints.access_service.tls", default=True)
                    timeout = config.get_value(
                        "endpoints.access_service.timeout",
                        default=DEFAULT_TIMEOUT,
                    )
                    retries = config.get_value(
                        "endpoints.access_service.retries",
                        default=DEFAULT_RETRIES,
                    )
                    keepalive_time = config.get_value(
                        "endpoints.access_service.keepalive_time",
                        default=DEFAULT_KEEPALIVE_TIME,
                    )
                    keepalive_timeout = config.get_value(
                        "endpoints.access_service.keepalive_timeout",
                        default=DEFAULT_KEEPALIVE_TIMEOUT,
                    )

                    user_agent = config.get_value(
                        "grpc.user_agent", default=None,
                    )

                    grpc_options = misc.drop_none({
                        "grpc.keepalive_time_ms": keepalive_time,
                        "grpc.keepalive_timeout_ms": keepalive_timeout,
                        "grpc.keepalive_permit_without_calls": 1,
                        "grpc.primary_user_agent": user_agent,
                    })
                    grpc_options = tuple(grpc_options.items())
                    if tls:
                        credentials = grpc.ssl_channel_credentials(
                            root_certificates=_read_root_certificates(),
                        )
                        channel = grpc.secure_channel(url, credentials, options=grpc_options)
                    else:
                        channel = grpc.insecure_channel(url, options=grpc_options)

                    if config.get_value("enable_grpc_metrics", default=False):
                        from yc_common.grpc_metrics import MetricClientInterceptor
                        channel = grpc.intercept_channel(channel, MetricClientInterceptor())

                    retry_policy = yc_as_client.client.YCAccessServiceRetryPolicy(
                        max_attemps=retries,
                    )
                    cls.__singleton_instance = cls(channel, timeout=timeout, retry_policy=retry_policy)

        return cls.__singleton_instance


def _read_root_certificates():
    cert_file_name = config.get_value("endpoints.access_service.ca_path", default=DEFAULT_CA_PATH)
    with open(cert_file_name, "rb") as f:
        certs = f.read()
    return certs


class AccessServiceEndpointConfig(models.Model):
    enabled = models.BooleanType(required=False)

    url = models.StringType(required=True)
    tls = models.BooleanType()
    timeout = models.FloatType()
