import datetime
import grpc
import random
import threading
import time

from yandex.cloud.priv.kms.v1 import symmetric_crypto_service_pb2
from yandex.cloud.priv.kms.v1 import symmetric_crypto_service_pb2_grpc


class KmsRoundRobinClientOptions(object):
    token_provider = None

    max_retries = 10
    initial_backoff = datetime.timedelta(milliseconds=10)
    max_backoff = datetime.timedelta(milliseconds=100)
    backoff_multiplier = 1.5
    jitter = 0.25

    per_try_duration = datetime.timedelta(seconds=1)
    keep_alive_time = datetime.timedelta(seconds=10)

    log = None

    shuffle_addrs = True

    root_ca_pem = None
    tls_server_name = None
    insecure = False


class KmsRoundRobinClient(object):
    def __init__(self, addrs, options=KmsRoundRobinClientOptions()):
        if len(addrs) == 0:
            raise ValueError("addrs must not be empty")
        self._addrs = list(addrs)
        if options.shuffle_addrs:
            # Shuffle addrs so that different clients will use different backend orders.
            random.shuffle(self._addrs)
        self._token_provider = options.token_provider
        self._initial_backoff = options.initial_backoff
        self._max_backoff = options.max_backoff
        self._backoff_multiplier = options.backoff_multiplier
        self._jitter = options.jitter
        self._max_retries = options.max_retries
        self._per_try_duration = options.per_try_duration.total_seconds()
        self._log = options.log
        self._last_index = 0
        self._last_index_lock = threading.Lock()

        channel_args = {
            'grpc.keepalive_time_ms': int(options.per_try_duration.total_seconds() * 1000),
            'grpc.keepalive_timeout_ms': int(options.keep_alive_time.total_seconds() * 1000),
        }
        if options.tls_server_name:
            channel_args['grpc.ssl_target_name_override'] = options.tls_server_name
        if options.root_ca_pem:
            creds = grpc.ssl_channel_credentials(root_certificates=options.root_ca_pem)
        else:
            creds = grpc.ssl_channel_credentials()

        self._channels = []
        self._stubs = []
        for addr in self._addrs:
            self._debug("Opening channel to %s", addr)
            if options.insecure:
                channel = grpc.insecure_channel(target=addr, options=channel_args.items())
            else:
                channel = grpc.secure_channel(target=addr, credentials=creds,
                                              options=channel_args.items())
            self._channels.append(channel)
            self._stubs.append(symmetric_crypto_service_pb2_grpc.SymmetricCryptoServiceStub(channel))

    def close(self):
        for channel in self._channels:
            channel.close()

    def encrypt(self, key_id, aad, plaintext, token=None):
        req = symmetric_crypto_service_pb2.SymmetricEncryptRequest(
            key_id=key_id,
            aad_context=aad,
            plaintext=plaintext
        )
        metadata = self._prepare_metadata(token)
        ret = self._do_call(lambda stub: stub.Encrypt(req, timeout=self._per_try_duration,
                            metadata=metadata))
        return ret.ciphertext

    def decrypt(self, key_id, aad, ciphertext, token=None):
        req = symmetric_crypto_service_pb2.SymmetricDecryptRequest(
            key_id=key_id,
            aad_context=aad,
            ciphertext=ciphertext
        )
        metadata = self._prepare_metadata(token)
        ret = self._do_call(lambda stub: stub.Decrypt(req, timeout=self._per_try_duration,
                            metadata=metadata))
        return ret.plaintext

    def _prepare_metadata(self, token):
        if token is None:
            token = self._token_provider()
        return [("authorization", "Bearer " + token)]

    def _do_call(self, call):
        with self._last_index_lock:
            stub_idx = self._last_index
            self._last_index += 1
            if self._last_index >= len(self._channels):
                self._last_index = 0

        next_sleep_sec = self._initial_backoff.total_seconds()
        max_sleep_sec = self._max_backoff.total_seconds()
        for num_retry in range(self._max_retries + 1):
            # NOTE: Unlike C++/Java/Go, there is no direct way to query the channel state,
            # so skip the "search for READY channels" part.
            try:
                return call(self._stubs[stub_idx])
            except grpc.RpcError as e:
                if not self._is_retryable(e) or num_retry == self._max_retries:
                    raise e

                sleep_sec = next_sleep_sec * self._jitter_factor()
                self._debug("Got status %s (%s) on retry %d (backend #%d (%s)), retrying in %dms",
                            e.code(), e.details(), num_retry, stub_idx, self._addrs[stub_idx],
                            int(sleep_sec * 1000))
                time.sleep(sleep_sec)
                next_sleep_sec = min(next_sleep_sec * self._backoff_multiplier, max_sleep_sec)
                stub_idx += 1
                if stub_idx >= len(self._channels):
                    stub_idx = 0

    def _is_retryable(self, e):
        code = e.code()
        return (code == grpc.StatusCode.DEADLINE_EXCEEDED
                or code == grpc.StatusCode.RESOURCE_EXHAUSTED
                or code == grpc.StatusCode.UNAVAILABLE)

    def _jitter_factor(self):
        return 1.0 - random.uniform(0.0, 1.0) * self._jitter

    def _debug(self, *l):
        if self._log:
            self._log.debug(*l)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
