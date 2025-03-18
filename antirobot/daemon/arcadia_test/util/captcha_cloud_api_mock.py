import yatest
import grpc
import time
import json

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess

import yandex.cloud.priv.smartcaptcha.v1.captcha_service_pb2_grpc as api_pb2_grpc
import yandex.cloud.priv.smartcaptcha.v1.stats_service_pb2_grpc as stats_api_pb2_grpc
import yandex.cloud.priv.smartcaptcha.v1.operation_service_pb2_grpc as operation_api_pb2_grpc
import yandex.cloud.priv.smartcaptcha.v1.operation_service_pb2 as operation_api_pb2
import yandex.cloud.priv.smartcaptcha.v1.captcha_pb2 as captcha_pb2
import yandex.cloud.priv.smartcaptcha.v1.captcha_service_pb2 as api_pb2
import yandex.cloud.priv.smartcaptcha.v1.quota_service_pb2_grpc as quota_service_pb2_grpc
import yandex.cloud.priv.quota.quota_pb2 as quota_pb2

from google.protobuf import any_pb2


class CaptchaCloudApi(NetworkSubprocess):
    def __init__(self, port, args, **kwargs):
        path = yatest.common.build_path(
            "antirobot/captcha_cloud_api/bin/captcha_cloud_api",
        )
        super().__init__(path, port, [
            "--addr", f":{port}",
            "--debug",
            "--resource-manager-insecure",
            "--access-service-insecure",
        ] + args, **kwargs)

        json_config = json.dumps({
            "methodConfig": [
                {
                    "name": [
                        {}
                    ],
                    "retryPolicy": {
                        "maxAttempts": 7,
                        "initialBackoff": "0.4s",
                        "maxBackoff": "3s",
                        "backoffMultiplier": 2,
                        "retryableStatusCodes": ["UNAVAILABLE"],
                    },
                }
            ]
        })

        channel = grpc.insecure_channel(self.host, options=[
            ('grpc.service_config', json_config),
            ('grpc.enable_retries', True),
            ('grpc.max_receive_message_length', 128 * 1024 * 1024),
            ('grpc.max_send_message_length', 128 * 1024 * 1024),
            ('grpc.primary_user_agent', "test"),
            ('grpc.initial_reconnect_backoff_ms', 100),
            ('grpc.max_reconnect_backoff_ms', 100),
            ('grpc.keepalive_time_ms', 100),
            ('grpc.keepalive_timeout_ms', 100),
            ('grpc.http2.min_time_between_pings_ms', 100),
            ('grpc.http2.min_ping_interval_without_data_ms', 100),
            ('grpc.primary_user_agent', "Python-Test-UA")
        ])
        self.stub = api_pb2_grpc.CaptchaSettingsServiceStub(channel)
        self.quota_stub = quota_service_pb2_grpc.QuotaServiceStub(channel)
        self.stats_stub = stats_api_pb2_grpc.StatsServiceStub(channel)
        self.operation_stub = operation_api_pb2_grpc.OperationServiceStub(channel)

        self.wait()
        time.sleep(5)  # ждём переподключения клиентов (TODO:tyamgin)

        self.test_captcha = self.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id="active-0123456789Fabcdefghij",
            name="test_captcha_for_demo",
            allowed_sites=["localhost"],
        ))

    def is_id(self, s):
        return s.startswith("xxx")

    def unpack_operation(self, operation, operation_type):
        assert self.is_id(operation.id), operation.id
        assert operation.description == operation_type.capitalize() + " captcha"
        assert operation.created_at.seconds > 1600000000
        assert operation.modified_at.seconds > 1600000000
        assert operation.created_by == "testing_user_account", operation.created_by
        assert operation.done is True
        assert isinstance(operation.response, any_pb2.Any)
        assert operation.response.type_url == "type.googleapis.com/yandex.cloud.priv.smartcaptcha.v1.CaptchaSettings"

        captcha = captcha_pb2.CaptchaSettings()
        operation.response.Unpack(captcha)

        if operation_type == "delete":
            assert operation.metadata.type_url == "type.googleapis.com/yandex.cloud.priv.smartcaptcha.v1.DeleteCaptchaMetadata"
            metadata = api_pb2.DeleteCaptchaMetadata()
        elif operation_type == "create":
            assert operation.metadata.type_url == "type.googleapis.com/yandex.cloud.priv.smartcaptcha.v1.CreateCaptchaMetadata"
            metadata = api_pb2.CreateCaptchaMetadata()
        elif operation_type == "update":
            assert operation.metadata.type_url == "type.googleapis.com/yandex.cloud.priv.smartcaptcha.v1.UpdateCaptchaMetadata"
            metadata = api_pb2.UpdateCaptchaMetadata()
        else:
            raise TypeError(f"Unknown operation_type '{operation_type}'")

        operation.metadata.Unpack(metadata)
        assert metadata.captcha_id == captcha.captcha_id

        return captcha

    def default_metadata(self):
        return [
            ("authorization", "bearer my-test-auth-token")
        ]

    def list_captchas(self, **kwargs):
        return self.stub.List(api_pb2.ListCaptchasRequest(**kwargs), metadata=self.default_metadata())

    def list_operations(self, **kwargs):
        return self.operation_stub.List(operation_api_pb2.ListOperationsRequest(**kwargs), metadata=self.default_metadata())

    def get_captcha(self, **kwargs):
        return self.stub.Get(api_pb2.GetSettingsRequest(**kwargs), metadata=self.default_metadata())

    def get_operation(self, **kwargs):
        return self.operation_stub.Get(operation_api_pb2.GetOperationRequest(**kwargs), metadata=self.default_metadata())

    def get_captcha_by_client_key(self, **kwargs):
        return self.stub.GetByClientKey(api_pb2.GetSettingsByClientKeyRequest(**kwargs), metadata=self.default_metadata())

    def get_captcha_by_server_key(self, **kwargs):
        return self.stub.GetByServerKey(api_pb2.GetSettingsByServerKeyRequest(**kwargs), metadata=self.default_metadata())

    def update_quota(self, **kwargs):
        return self.quota_stub.UpdateMetric(quota_pb2.UpdateQuotaMetricRequest(**kwargs), metadata=self.default_metadata())

    def get_quota(self, **kwargs):
        return self.quota_stub.Get(quota_pb2.GetQuotaRequest(**kwargs), metadata=self.default_metadata())

    def batch_update_quota(self, **kwargs):
        return self.quota_stub.BatchUpdateMetric(quota_pb2.BatchUpdateQuotaMetricsRequest(**kwargs), metadata=self.default_metadata())

    def create_captcha(self, request):
        new_captcha_op = self.stub.Create(request, metadata=self.default_metadata())
        new_captcha = self.unpack_operation(new_captcha_op, "create")

        assert new_captcha.name == request.name
        assert new_captcha.folder_id == request.folder_id
        assert self.is_id(new_captcha.captcha_id), new_captcha.captcha_id
        assert len(new_captcha.captcha_id) == 20, new_captcha.captcha_id
        assert len(new_captcha.client_key) == 40
        assert len(new_captcha.server_key) == 40
        assert new_captcha.client_key[:20] == new_captcha.server_key[:20]
        assert new_captcha.allowed_sites == request.allowed_sites
        assert new_captcha.complexity == request.complexity
        assert new_captcha.created_at.seconds > 1600000000
        assert new_captcha.created_at == new_captcha.updated_at

        return new_captcha

    def update_captcha(self, request, prev_captcha):
        update_captcha_op = self.stub.Update(request, metadata=self.default_metadata())
        captcha = self.unpack_operation(update_captcha_op, "update")

        fields = [f.name for f in api_pb2.UpdateCaptchaRequest.DESCRIPTOR.fields if f.name not in ("captcha_id", "update_mask")]
        for field in fields:
            if field in request.update_mask.paths:
                assert getattr(captcha, field) == getattr(request, field)
            else:
                assert getattr(captcha, field) == getattr(prev_captcha, field)

        # compare other fields:
        assert prev_captcha.folder_id == captcha.folder_id
        assert prev_captcha.client_key == captcha.client_key
        assert prev_captcha.server_key == captcha.server_key
        assert prev_captcha.created_at == captcha.created_at
        assert (prev_captcha.created_at.seconds, prev_captcha.created_at.nanos) < (captcha.updated_at.seconds, captcha.updated_at.nanos)

        return captcha

    def update_captcha_all(self, request, prev_captcha):
        update_captcha_op = self.stub.UpdateAll(request, metadata=self.default_metadata())
        captcha = self.unpack_operation(update_captcha_op, "update")

        fields = [f.name for f in captcha.DESCRIPTOR.fields]
        for field in fields:
            if field in request.update_mask.paths:
                assert getattr(captcha, field) == getattr(request.settings, field)
            else:
                assert getattr(captcha, field) == getattr(prev_captcha, field)

        return captcha

    def delete_captcha(self, request, prev_captcha):
        delete_captcha_op = self.stub.Delete(request, metadata=self.default_metadata())
        captcha = self.unpack_operation(delete_captcha_op, "delete")

        # compare other fields:
        assert prev_captcha.folder_id == captcha.folder_id
        assert prev_captcha.client_key == captcha.client_key
        assert prev_captcha.server_key == captcha.server_key
        assert prev_captcha.created_at == captcha.created_at
        assert prev_captcha.name == captcha.name
        assert prev_captcha.allowed_sites == captcha.allowed_sites
        assert prev_captcha.complexity == captcha.complexity
        assert (prev_captcha.created_at.seconds, prev_captcha.created_at.nanos) < (captcha.updated_at.seconds, captcha.updated_at.nanos)

        return captcha
