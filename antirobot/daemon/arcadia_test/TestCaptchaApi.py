import urllib
import json
import string
import random
import re
import grpc
import pytest
import time
import dateutil.parser

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
)

import yandex.cloud.priv.smartcaptcha.v1.captcha_pb2 as captcha_pb2
import yandex.cloud.priv.smartcaptcha.v1.captcha_service_pb2 as api_pb2
import yandex.cloud.priv.smartcaptcha.v1.stats_service_pb2 as stats_api_pb2
import yandex.cloud.priv.smartcaptcha.v1.operation_service_pb2 as operation_api_pb2
import yandex.cloud.priv.quota.quota_pb2 as quota_pb2

from google.protobuf import (
    empty_pb2,
    field_mask_pb2,
)
from grpc_status import rpc_status


PERMISSION_DENIED_AUDIT_ERROR = {
    "code": 7,
    "message": "Permission denied",
    "details": [
        {
            "@type": "type.googleapis.com/google.rpc.LocalizedMessage",
            "locale": "en",
            "message": "Permission denied"
        }
    ],
}


def generate_id(size=10, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def generate_cloud_id(status="active"):
    return status + "-" + generate_id(10, chars=string.digits)


def generate_folder_id(cloud_id):
    return cloud_id + "F" + generate_id(10, chars=string.ascii_lowercase)


class TestCaptchaApiBase(AntirobotTestSuite):
    captcha_args = ["--generate", "correct"]
    options = {
        "DebugOutput": 1,
        "AsCaptchaApiService": 1,
        "CaptchaCheckTimeout": "0.05s",
        "CaptchaGenTryCount": 3,
        "EventLogFrameSize": 0,  # все события сразу пишутся в файл eventlog'а
        "FuryBaseTimeout": "0.1s",
        "CloudCaptchaApiKeepAliveTime": "100ms",
        "CloudCaptchaApiKeepAliveTimeout": "100ms",
    }

    def send_check_request(self, data, ip):
        return self.send_request(
            urllib.request.Request(
                f"http://{self.antirobot.host}/check",
                headers={"X-Forwarded-For-Y": ip},
                data=urllib.parse.urlencode(data).encode(),
                method="POST"
            )
        )

    def send_validate_request(self, spravka, ip, now=None, secret=None, tvm_ticket=None, req_id=None):
        headers = {"X-Forwarded-For-Y": "1.2.3.4"}  # validator's IP is different from user's IP
        if tvm_ticket is not None:
            headers["X-Ya-Service-Ticket"] = tvm_ticket
        if req_id is not None:
            headers["X-Req-Id"] = req_id
        if now is not None:
            headers["X-Start-Time"] = str(now * 1000000)
        url = f"http://{self.antirobot.host}/validate?token={urllib.parse.quote(spravka)}&ip={ip}"
        if secret is not None:
            url += f"&secret={secret}"
        return self.send_request(
            urllib.request.Request(
                url,
                headers=headers,
                method="GET"
            )
        )

    def get_iframe_url(self):
        resp = self.send_request(
            urllib.request.Request(
                f"http://{self.antirobot.host}/captcha.js",
                headers={
                    "X-Forwarded-For-Y": "1.2.3.4",
                },
                method="GET"
            ),
        )
        assert resp.getcode() == 200
        path = re.match(re.compile("^.*(/checkbox\\.\\w+\\.html).*$", re.DOTALL), resp.read().decode()).groups()[0]
        return f"http://{self.antirobot.host}{path}"

    def get_next_captcha(self, ip, client_key=None):
        data = {}
        if client_key is not None:
            data["sitekey"] = client_key
        resp = self.send_check_request(data, ip)

        assert resp.getcode() == 200
        content = json.loads(resp.read().decode())

        data["key"] = content["captcha"]["key"]
        data["d"] = content["captcha"]["d"]
        data["k"] = content["captcha"]["k"]
        resp = self.send_check_request(data, ip)

        assert resp.getcode() == 200
        return json.loads(resp.read().decode())

    def get_next_captcha2(self, ip, content, client_key=None):
        data = {}
        if client_key is not None:
            data["sitekey"] = client_key

        data["key"] = content["captcha"]["key"]
        data["d"] = content["captcha"]["d"]
        data["k"] = content["captcha"]["k"]
        data["rep"] = "rep"
        resp = self.send_check_request(data, ip)

        assert resp.getcode() == 200
        return json.loads(resp.read().decode())

    def get_spravka(self, ip, client_key=None):
        self.fury.set_strategy("ButtonStrategy", "success")
        content = self.get_next_captcha(ip, client_key)
        return content["spravka"]


class TestCaptchaApi(TestCaptchaApiBase):
    ydb_args = []

    def test_pass_captcha(self):
        ip = GenRandomIP()

        self.fury.set_strategy("ButtonStrategy", "fail")
        resp = self.send_check_request({}, ip)

        assert resp.getcode() == 200
        content = json.loads(resp.read().decode())
        assert content["status"] == "failed"
        assert content["captcha"]["key"]
        assert content["captcha"]["type"] == "checkbox"

        resp = self.send_check_request({
            "key": content["captcha"]["key"],
            "d": content["captcha"]["d"],
            "k": content["captcha"]["k"],
        }, ip)

        assert resp.getcode() == 200
        content = json.loads(resp.read().decode())
        assert content["status"] == "failed"
        assert content["captcha"]["key"]
        assert content["captcha"]["voice"]
        assert content["captcha"]["voiceintro"]
        assert content["captcha"]["image"]
        assert content["captcha"]["type"] == "image"

        self.captcha.set_strategy("CheckStrategy", "as_rep")
        self.fury.set_strategy("Strategy", "as_image")
        resp = self.send_check_request({
            "key": content["captcha"]["key"],
            "d": content["captcha"]["d"],
            "k": content["captcha"]["k"],
            "rep": "ok"
        }, ip)
        assert resp.getcode() == 200
        content = json.loads(resp.read().decode())
        assert content["status"] == "ok"
        assert content["spravka"]

    def test_validate_true(self):
        ip = GenRandomIP()
        spravka = self.get_spravka(ip)

        resp = self.send_validate_request(spravka, ip, now=1900000000, tvm_ticket="test")
        content = json.loads(resp.read().decode())
        assert content["status"] == "failed", content  # expired
        assert content["message"] == "Token invaid or expired.", content

        for i in range(3):
            resp = self.send_validate_request(spravka, ip, tvm_ticket="test")
            content = json.loads(resp.read().decode())
            if i == 0:
                assert content["status"] == "ok", content
            else:
                assert content["status"] == "failed", (i, content)

    def test_validate_failed(self):
        ip = GenRandomIP()
        resp = self.send_validate_request("invalid_spravka", ip)
        content = json.loads(resp.read().decode())
        assert content["status"] == "failed"


class TestCaptchaApiWithoutYdb(TestCaptchaApiBase):
    def test_validate_ydb_unavailable(self):
        ip = GenRandomIP()
        spravka = self.get_spravka(ip)

        for i in range(3):
            resp = self.send_validate_request(spravka, ip, tvm_ticket="test")
            content = json.loads(resp.read().decode())
            assert content["status"] == "ok", (i, content)


class TestCaptchaCloudApiBase(TestCaptchaApiBase):
    ydb_args = []
    captcha_cloud_api_args = ["--default-captchas-quota", "100"]


class TestCaptchaCloudApi(TestCaptchaCloudApiBase):
    num_antirobots = 0

    def test_create_and_get_by_captcha_id(self):
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
        ))
        captcha = self.captcha_cloud_api.get_captcha(captcha_id=new_captcha.captcha_id)
        assert new_captcha == captcha

    @pytest.mark.parametrize("cloud_status, expected_suspend", [
        ("active",  False),
        ("blocked", True),
    ])
    def test_create_and_get_by_key(self, cloud_status, expected_suspend):
        cloud_id = generate_cloud_id(cloud_status)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="my_first_captcha",
        ))
        captcha_by_server_key = self.captcha_cloud_api.stub.GetByServerKey(
            api_pb2.GetSettingsByServerKeyRequest(server_key=new_captcha.server_key),
            metadata=[]
        )
        captcha_by_client_key = self.captcha_cloud_api.stub.GetByClientKey(
            api_pb2.GetSettingsByClientKeyRequest(client_key=new_captcha.client_key),
            metadata=[]
        )
        for captcha in (captcha_by_server_key, captcha_by_client_key):
            assert expected_suspend == captcha.suspend, captcha

            captcha.suspend = new_captcha.suspend
            assert new_captcha == captcha

    def test_get_unexisten(self):
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.get_captcha(captcha_id="some_unexisten_key")
        assert exception_info.value.code() == grpc.StatusCode.NOT_FOUND

    def test_operation_get(self):
        cloud_id = generate_cloud_id()
        new_captcha_op = self.captcha_cloud_api.stub.Create(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
        ), metadata=self.captcha_cloud_api.default_metadata())

        from_db_op = self.captcha_cloud_api.get_operation(
            operation_id=new_captcha_op.id,
        )
        assert new_captcha_op == from_db_op

    def test_operation_get_if_not_exists(self):
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.get_operation(
                operation_id="xxxyyyyyyyyyyyyyyyyy",
            )
        assert exception_info.value.code() == grpc.StatusCode.NOT_FOUND

    def test_operation_list(self):
        cloud_id = generate_cloud_id()
        folder_id1 = generate_folder_id(cloud_id)
        folder_id2 = generate_folder_id(cloud_id)
        folder_id3 = generate_folder_id(cloud_id)
        operations = []
        for folder_id in (folder_id1, folder_id2, folder_id1, folder_id2):
            operations.append(self.captcha_cloud_api.stub.Create(
                api_pb2.CreateCaptchaRequest(folder_id=folder_id),
                metadata=self.captcha_cloud_api.default_metadata()
            ))

        folder_1_list = self.captcha_cloud_api.list_operations(
            folder_id=folder_id1,
        )
        assert folder_1_list == operation_api_pb2.ListOperationsResponse(
            operations=[operations[2], operations[0]],
        )

        folder_1_list = self.captcha_cloud_api.list_operations(
            folder_id=folder_id1,
            page_size=1,
        )
        assert folder_1_list == operation_api_pb2.ListOperationsResponse(
            operations=[operations[2]],
        )

        folder_3_list = self.captcha_cloud_api.list_operations(
            folder_id=folder_id3,
        )
        assert folder_3_list == operation_api_pb2.ListOperationsResponse(
            operations=[],
        )

    def test_operation_when_delete(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha_op = self.captcha_cloud_api.stub.Create(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
        ), metadata=self.captcha_cloud_api.default_metadata())
        delete_captcha_op = self.captcha_cloud_api.stub.Delete(api_pb2.DeleteCaptchaRequest(
            captcha_id=self.captcha_cloud_api.unpack_operation(new_captcha_op, "create").captcha_id
        ), metadata=self.captcha_cloud_api.default_metadata())

        operations = self.captcha_cloud_api.list_operations(
            folder_id=folder_id,
        )
        assert operations == operation_api_pb2.ListOperationsResponse(
            operations=[delete_captcha_op, new_captcha_op],
        )

    def test_update_if_not_exists(self):
        with pytest.raises(grpc.RpcError) as exception_info:
            prev_captcha = None
            self.captcha_cloud_api.update_captcha(api_pb2.UpdateCaptchaRequest(
                captcha_id="xxxxxxyyyyyyyyzzzzzz",
                name="New name",
                allowed_sites=["new-site.yandex"]
            ), prev_captcha)
        assert exception_info.value.code() == grpc.StatusCode.NOT_FOUND

    @pytest.mark.parametrize("mask", [
        ["name", "allowed_sites"],
        ["name"],
        ["allowed_sites"],
        ["complexity"],
        [],
        None,
    ])
    def test_update_1(self, mask):
        if mask is None:
            mask = [f.name for f in api_pb2.UpdateCaptchaRequest.DESCRIPTOR.fields if f.name not in ("captcha_id", "update_mask")]

        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        first_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="First",
            allowed_sites=["yandex.ru"]
        ))
        prev_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        update_request = api_pb2.UpdateCaptchaRequest(
            captcha_id=prev_captcha.captcha_id,
            name="New name",
            allowed_sites=["new-site.yandex"],
            complexity=captcha_pb2.CaptchaComplexity.MEDIUM,
            style_json='{"b":1}',
            update_mask=field_mask_pb2.FieldMask(paths=mask)
        )
        for field in mask:
            assert getattr(update_request, field) != getattr(api_pb2.UpdateCaptchaRequest(), field), f"Please set not default field '{field}' in update_request"

        updated_captcha = self.captcha_cloud_api.update_captcha(update_request, prev_captcha)

        captchas = self.captcha_cloud_api.list_captchas(folder_id=folder_id)
        assert captchas == api_pb2.ListCaptchasResponse(
            captchas=[first_captcha, updated_captcha],
        )

    @pytest.mark.parametrize("mask", [
        ["name", "allowed_sites"],
        ["name"],
        ["allowed_sites"],
        ["complexity"],
        ["captcha_id", "client_key", "is_yandex_client", "style_json"],
        [],
    ])
    def test_update_all(self, mask):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)

        prev_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        self.captcha_cloud_api.update_captcha_all(api_pb2.UpdateAllCaptchaRequest(
            captcha_id=prev_captcha.captcha_id,
            settings=captcha_pb2.CaptchaSettings(
                captcha_id="new_id",
                client_key="new_clienk_key",
                is_yandex_client=True,
                style_json='{"a":1}',
                name="New name",
                allowed_sites=["new-site.yandex"],
                complexity=captcha_pb2.CaptchaComplexity.MEDIUM
            ),
            update_mask=field_mask_pb2.FieldMask(paths=mask)
        ), prev_captcha)

    def test_update_all_bad(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)

        prev_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
        ))
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.update_captcha_all(api_pb2.UpdateAllCaptchaRequest(
                captcha_id=prev_captcha.captcha_id,
                settings=captcha_pb2.CaptchaSettings(),
                update_mask=field_mask_pb2.FieldMask(paths=["qweqweqwe"])
            ), prev_captcha)
        assert exception_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT

    def test_delete_unexisten(self):
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.delete_captcha(api_pb2.DeleteCaptchaRequest(captcha_id="some_unexisten_key"), None)
        assert exception_info.value.code() == grpc.StatusCode.NOT_FOUND

    def test_delete_1(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="Will be deleted",
            allowed_sites=["yandex.ru"]
        ))
        self.captcha_cloud_api.delete_captcha(api_pb2.DeleteCaptchaRequest(captcha_id=captcha.captcha_id), captcha)
        captchas = self.captcha_cloud_api.list_captchas(folder_id=folder_id)
        assert captchas == api_pb2.ListCaptchasResponse(captchas=[])

    def test_max_size_allowed_sites(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=folder_id,
                name="test",
                allowed_sites=["superpupersite.example.com"]*1000
            ))
        assert exception_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT
        assert "Sites list is too large" in str(exception_info.value)

    def test_max_size_style_json(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=folder_id,
                name="test",
                allowed_sites=["superpupersite.example.com"]*100,
                style_json="{"+" "*10000+"}"
            ))
        assert exception_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT
        assert "StyleJson is too large" in str(exception_info.value)

    def test_search_topic(self):
        now_timestamp = int(time.time())
        self.unified_agent.captcha_cloud_api_search_topic_log.pop_logs()
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="find_me1",
        ))
        captcha = self.captcha_cloud_api.get_captcha(captcha_id=new_captcha.captcha_id)
        assert new_captcha == captcha

        time.sleep(1)
        logs = self.unified_agent.captcha_cloud_api_search_topic_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["resource_type"] == "captcha"
        assert ev["resource_id"] == new_captcha.captcha_id
        assert ev["name"] == new_captcha.name
        assert ev["service"] == "smart-captcha"
        assert ev["permission"] == "smart-captcha.captchas.get"
        assert ev["cloud_id"] == new_captcha.cloud_id
        assert ev["folder_id"] == new_captcha.folder_id
        assert ev["resource_path"] == [
            {'resource_type': 'resource-manager.cloud', 'resource_id': new_captcha.cloud_id},
            {'resource_type': 'resource-manager.folder', 'resource_id': new_captcha.folder_id},
            {'resource_type': 'smart-captcha.captcha', 'resource_id': new_captcha.captcha_id}
        ]
        assert "reindex_timestamp" not in ev
        assert "deleted" not in ev
        assert now_timestamp <= int(dateutil.parser.isoparse(ev["timestamp"]).strftime("%s")) <= now_timestamp + 30

        self.unified_agent.captcha_cloud_api_search_topic_log.pop_logs()
        updated_captcha = self.captcha_cloud_api.update_captcha(api_pb2.UpdateCaptchaRequest(
            captcha_id=new_captcha.captcha_id,
            name="find_me2",
            update_mask=field_mask_pb2.FieldMask(paths=["name"])
        ), new_captcha)
        time.sleep(1)
        logs = self.unified_agent.captcha_cloud_api_search_topic_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["name"] == updated_captcha.name
        assert ev["service"] == "smart-captcha"
        assert "reindex_timestamp" not in ev
        assert "deleted" not in ev
        assert now_timestamp <= int(dateutil.parser.isoparse(ev["timestamp"]).strftime("%s")) <= now_timestamp + 30

        self.unified_agent.captcha_cloud_api_search_topic_log.pop_logs()
        self.captcha_cloud_api.delete_captcha(api_pb2.DeleteCaptchaRequest(captcha_id=updated_captcha.captcha_id), updated_captcha)
        time.sleep(1)
        logs = self.unified_agent.captcha_cloud_api_search_topic_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["name"] == updated_captcha.name
        assert ev["service"] == "smart-captcha"
        assert "reindex_timestamp" not in ev
        assert now_timestamp <= int(dateutil.parser.isoparse(ev["timestamp"]).strftime("%s")) <= now_timestamp + 30
        assert ev["timestamp"] == ev["deleted"]

    def test_create_audit_log(self):
        self.unified_agent.audit_log.pop_logs()
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_user_account',
        }
        assert ev["authorization"] == {
            "authorized": True,
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.create",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                    "authorized": True,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.CreateCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
            "cloud_id": cloud_id,
            "folder_id": folder_id,
        }
        assert ev["request_metadata"] == {
            "remote_address": "::1",
            "user_agent": ev["request_metadata"]["user_agent"],
            "request_id": ev["request_metadata"]["request_id"],
        }
        assert "Python-Test-UA" in ev["request_metadata"]["user_agent"]
        assert ev["request_metadata"]["request_id"].startswith("x-")
        assert ev["event_status"] == "DONE"
        assert ev["details"] == {
            "captcha_id": new_captcha.captcha_id,
            "client_key": new_captcha.client_key,
            "server_key": "***hidden***",
            "name": new_captcha.name,
            "allowed_sites": new_captcha.allowed_sites,
            "complexity": "HARD",
        }
        assert ev["request_parameters"] == {
            "folder_id": new_captcha.folder_id,
            "name": new_captcha.name,
            "allowed_sites": new_captcha.allowed_sites,
            "complexity": "HARD",
        }
        assert ev["response"]["operation_id"].startswith("xxx")

    def test_create_unauthorized_audit_log(self):
        self.unified_agent.audit_log.pop_logs()
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)

        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.stub.Create(
                api_pb2.CreateCaptchaRequest(
                    folder_id=folder_id,
                    name="my_first_captcha",
                    allowed_sites=["site.com"],
                    complexity=captcha_pb2.CaptchaComplexity.EASY,
                ),
                metadata=[
                    ("authorization", "bearer my-test-no-permissions-auth-token"),
                    ("user-agent", "test-ua"),
                    ("x-request-id", "test-req-id"),
                    ("idempotency-key", "test-idempotency-key"),
                    ("x-forwarded-for", "1.2.3.4"),
                ]
            )
        assert exception_info.value.code() == grpc.StatusCode.PERMISSION_DENIED

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        import sys
        print(ev, file=sys.stderr)
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_no_permissions_user_account',
        }
        assert ev["authorization"] == {
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.create",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.CreateCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
            "folder_id": folder_id,
        }
        assert ev["request_metadata"] == {
            "remote_address": "1.2.3.4",
            "user_agent": ev["request_metadata"]["user_agent"],
            "request_id": "test-req-id",
            "idempotency_id": "test-idempotency-key",
        }
        assert ev["event_status"] == "ERROR"
        assert ev["details"] == {}
        assert ev["error"] == PERMISSION_DENIED_AUDIT_ERROR
        assert ev["request_parameters"] == {
            "folder_id": folder_id,
            "name": "my_first_captcha",
            "allowed_sites": ["site.com"],
            "complexity": "EASY",
        }
        assert ev["response"] == {}

    def test_update_audit_log(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        self.unified_agent.audit_log.pop_logs()

        updated_captcha = self.captcha_cloud_api.update_captcha(api_pb2.UpdateCaptchaRequest(
            captcha_id=new_captcha.captcha_id,
            name="find_me2",
            complexity=captcha_pb2.CaptchaComplexity.EASY,
            update_mask=field_mask_pb2.FieldMask(paths=["name", "complexity"])
        ), new_captcha)

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_user_account',
        }
        assert ev["authorization"] == {
            "authorized": True,
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.update",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                    "authorized": True,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.UpdateCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
            "cloud_id": cloud_id,
            "folder_id": folder_id,
        }
        assert ev["event_status"] == "DONE"
        assert ev["details"] == {
            "captcha_id": updated_captcha.captcha_id,
            "client_key": updated_captcha.client_key,
            "server_key": "***hidden***",
            "name": updated_captcha.name,
            "allowed_sites": updated_captcha.allowed_sites,
            "complexity": "EASY",
        }
        assert ev["request_parameters"] == {
            "captcha_id": updated_captcha.captcha_id,
            "update_mask": "name,complexity",
            "name": updated_captcha.name,
            "complexity": "EASY",
        }
        assert ev["response"]["operation_id"].startswith("xxx")

    def test_update_unauthorized_audit_log(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        self.unified_agent.audit_log.pop_logs()

        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.stub.Update(
                api_pb2.UpdateCaptchaRequest(
                    captcha_id=new_captcha.captcha_id,
                    name="find_me2",
                    complexity=captcha_pb2.CaptchaComplexity.EASY,
                    update_mask=field_mask_pb2.FieldMask(paths=["name", "complexity"])
                ),
                metadata=[
                    ("authorization", "bearer my-test-no-permissions-auth-token"),
                ]
            )
        assert exception_info.value.code() == grpc.StatusCode.PERMISSION_DENIED

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_no_permissions_user_account',
        }
        assert ev["authorization"] == {
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.update",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                }
            ]
        }
        assert ev["event_status"] == "ERROR"
        assert ev["details"] == {
            "captcha_id": new_captcha.captcha_id,
            "client_key": new_captcha.client_key,
            "server_key": "***hidden***",
            "name": new_captcha.name,
            "allowed_sites": new_captcha.allowed_sites,
            "complexity": "HARD",
        }
        assert ev["error"] == PERMISSION_DENIED_AUDIT_ERROR
        assert ev["request_parameters"] == {
            "captcha_id": new_captcha.captcha_id,
            "update_mask": "name,complexity",
            "name": "find_me2",
            "complexity": "EASY",
        }
        assert ev["response"] == {}

    def test_update_all_audit_log(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        self.unified_agent.audit_log.pop_logs()

        update_mask = ["client_key", "is_yandex_client", "style_json", "name", "allowed_sites", "complexity"]
        update_request = api_pb2.UpdateAllCaptchaRequest(
            captcha_id=new_captcha.captcha_id,
            settings=captcha_pb2.CaptchaSettings(
                captcha_id="new_id",
                client_key="new_clienk_key",
                server_key="new_server_key",
                is_yandex_client=True,
                style_json='{"a":1}',
                name="New name",
                allowed_sites=["new-site.yandex"],
                complexity=captcha_pb2.CaptchaComplexity.MEDIUM
            ),
            update_mask=field_mask_pb2.FieldMask(paths=update_mask)
        )
        updated_captcha = self.captcha_cloud_api.update_captcha_all(update_request, new_captcha)

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_user_account',
        }
        assert ev["authorization"] == {
            "authorized": True,
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.update",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                    "authorized": True,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.UpdateCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
            "cloud_id": cloud_id,
            "folder_id": folder_id,
        }
        assert ev["event_status"] == "DONE"
        assert ev["details"] == {
            "captcha_id": updated_captcha.captcha_id,
            "client_key": updated_captcha.client_key,
            "server_key": "***hidden***",
            "name": updated_captcha.name,
            "allowed_sites": updated_captcha.allowed_sites,
            "complexity": "MEDIUM",
            "style_json": updated_captcha.style_json,
        }
        assert ev["request_parameters"] == {
            "client_key": update_request.settings.client_key,
            "server_key": "***hidden***",
            "captcha_id": update_request.captcha_id,
            "update_mask": "clientKey,isYandexClient,styleJson,name,allowedSites,complexity",
            "name": update_request.settings.name,
            "complexity": "MEDIUM",
            "style_json": update_request.settings.style_json,
            "allowed_sites": update_request.settings.allowed_sites,
        }
        assert ev["response"]["operation_id"].startswith("xxx")

    def test_delete_audit_log(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        self.unified_agent.audit_log.pop_logs()

        self.captcha_cloud_api.delete_captcha(api_pb2.DeleteCaptchaRequest(
            captcha_id=new_captcha.captcha_id,
        ), new_captcha)

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_user_account',
        }
        assert ev["authorization"] == {
            "authorized": True,
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.delete",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                    "authorized": True,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.DeleteCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
            "cloud_id": cloud_id,
            "folder_id": folder_id,
        }
        assert ev["event_status"] == "DONE"
        assert ev["details"] == {
            "captcha_id": new_captcha.captcha_id,
            "client_key": new_captcha.client_key,
            "server_key": "***hidden***",
            "name": new_captcha.name,
            "allowed_sites": new_captcha.allowed_sites,
            "complexity": "HARD",
        }
        assert ev["request_parameters"] == {
            "captcha_id": new_captcha.captcha_id,
        }
        assert ev["response"]["operation_id"].startswith("xxx")

    def test_delete_unauthorized_audit_log(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=folder_id,
            name="my_first_captcha",
            allowed_sites=["yandex.kz", "yandex.ru"],
            complexity=captcha_pb2.CaptchaComplexity.HARD,
        ))
        time.sleep(1)
        self.unified_agent.audit_log.pop_logs()

        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.stub.Delete(
                api_pb2.DeleteCaptchaRequest(
                    captcha_id=new_captcha.captcha_id,
                ),
                metadata=[
                    ("authorization", "bearer my-test-no-permissions-auth-token"),
                ]
            )
        assert exception_info.value.code() == grpc.StatusCode.PERMISSION_DENIED

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_no_permissions_user_account',
        }
        assert ev["authorization"] == {
            "permissions": [
                {
                    "permission": "smart-captcha.captchas.delete",
                    "resource_type": "resource-manager.folder",
                    "resource_id": folder_id,
                }
            ]
        }
        assert ev["event_metadata"] == {
            "event_id": ev["event_metadata"]["event_id"],
            "event_type": "yandex.cloud.events.smartcaptcha.DeleteCaptcha",
            "created_at": ev["event_metadata"]["created_at"],
        }
        assert ev["event_status"] == "ERROR"
        assert ev["details"] == {
            "captcha_id": new_captcha.captcha_id,
            "client_key": new_captcha.client_key,
            "server_key": "***hidden***",
            "name": new_captcha.name,
            "allowed_sites": new_captcha.allowed_sites,
            "complexity": "HARD",
        }
        assert ev["error"] == PERMISSION_DENIED_AUDIT_ERROR
        assert ev["request_parameters"] == {
            "captcha_id": new_captcha.captcha_id,
        }
        assert ev["response"] == {}

    def test_delete_unexisten_audit_log(self):
        captcha_id = "zzz"
        self.unified_agent.audit_log.pop_logs()

        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.stub.Delete(
                api_pb2.DeleteCaptchaRequest(captcha_id=captcha_id),
                metadata=self.captcha_cloud_api.default_metadata(),
            )
        assert exception_info.value.code() == grpc.StatusCode.NOT_FOUND

        time.sleep(1)
        logs = self.unified_agent.audit_log.get_logs()
        assert len(logs) == 1, str(logs)
        ev = logs[0]
        assert ev["authentication"] == {
            "authenticated": True,
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
            "subject_id": 'testing_user_account',
        }
        assert ev["authorization"] == {}
        assert ev["event_metadata"]["event_type"] == "yandex.cloud.events.smartcaptcha.DeleteCaptcha"
        assert ev["event_status"] == "ERROR"
        assert ev["details"] == {}
        assert ev["error"] == {
            "code": 5,
            "message": "Item does not exists"
        }
        assert ev["request_parameters"] == {
            "captcha_id": captcha_id,
        }
        assert ev["response"] == {}


class TestCaptchaCloudApiCaptchasQuota(TestCaptchaCloudApiBase):
    num_antirobots = 0
    DEFAULT_CAPTCHAS_LIMIT = 3
    captcha_cloud_api_args = ["--default-captchas-quota", str(DEFAULT_CAPTCHAS_LIMIT)]

    def test_quota_service_get_default(self):
        res = self.captcha_cloud_api.quota_stub.GetDefault(quota_pb2.GetQuotaDefaultRequest())
        assert res == quota_pb2.GetQuotaDefaultResponse(
            metrics=[
                quota_pb2.MetricLimit(
                    name="smart-captcha.captchas.count",
                    limit=self.DEFAULT_CAPTCHAS_LIMIT,
                )
            ]
        )

    def test_update_quota_invalid(self):
        cloud_id = generate_cloud_id()
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.update_quota(
                cloud_id=cloud_id,
                metric=quota_pb2.MetricLimit(
                    name="hello.world",
                    limit=5,
                )
            )
        assert exception_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT

    @pytest.mark.parametrize("usage", [
        0,
        2,
    ])
    def test_update_quota_captchas_count(self, usage):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        for _ in range(usage):
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=folder_id,
                name="Bum",
                allowed_sites=["example.com", "example.ru"],
                complexity=captcha_pb2.CaptchaComplexity.HARD,
            ))

        for val in (0, 70, 80):
            if val > 0:
                assert empty_pb2.Empty() == self.captcha_cloud_api.update_quota(
                    cloud_id=cloud_id,
                    metric=quota_pb2.MetricLimit(
                        name="smart-captcha.captchas.count",
                        limit=val,
                    )
                )
            rec = self.captcha_cloud_api.get_quota(cloud_id=cloud_id)
            exp = quota_pb2.Quota(
                cloud_id=cloud_id,
                metrics=[
                    quota_pb2.QuotaMetric(
                        name="smart-captcha.captchas.count",
                        limit=self.DEFAULT_CAPTCHAS_LIMIT if val == 0 else val,
                        usage=usage,
                    )
                ]
            )
            assert rec == exp

    def test_stats_service(self):
        cloud_id = generate_cloud_id()
        folder_id_1 = generate_folder_id(cloud_id)
        folder_id_2 = generate_folder_id(cloud_id)
        expected_captchas_count = {
            folder_id_1: 2,
            folder_id_2: 1,
        }
        for folder_id in (folder_id_1, folder_id_2):
            for _ in range(expected_captchas_count[folder_id]):
                self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                    folder_id=folder_id,
                    name="Bum",
                    allowed_sites=["example.com", "example.ru"],
                    complexity=captcha_pb2.CaptchaComplexity.HARD,
                ))

        for folder_id in (folder_id_1, folder_id_2):
            resp = self.captcha_cloud_api.stats_stub.Folder(
                stats_api_pb2.GetFolderStatsRequest(folder_id=folder_id),
                metadata=self.captcha_cloud_api.default_metadata()
            )
            assert resp.total_instances == expected_captchas_count[folder_id]

    def test_stats_service_unauthenticated(self):
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.stats_stub.Folder(stats_api_pb2.GetFolderStatsRequest(folder_id="asdasd"))
        assert exception_info.value.code() == grpc.StatusCode.UNAUTHENTICATED

    def test_batch_update_quota_invalid(self):
        cloud_id = generate_cloud_id()
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.batch_update_quota(
                cloud_id=cloud_id,
                metrics=[
                    quota_pb2.MetricLimit(
                        name="hello.world",
                        limit=5,
                    ),
                ]
            )
        assert exception_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT

    def test_batch_update_quota_1(self):
        cloud_id = generate_cloud_id()
        self.captcha_cloud_api.batch_update_quota(
            cloud_id=cloud_id,
            metrics=[
                quota_pb2.MetricLimit(
                    name="smart-captcha.captchas.count",
                    limit=555,
                ),
            ]
        )
        assert self.captcha_cloud_api.get_quota(cloud_id=cloud_id) == quota_pb2.Quota(
            cloud_id=cloud_id,
            metrics=[
                quota_pb2.QuotaMetric(
                    name="smart-captcha.captchas.count",
                    limit=555,
                    usage=0.0,
                )
            ]
        )

    def test_get_quota_default(self):
        cloud_id = generate_cloud_id()
        assert self.captcha_cloud_api.get_quota(cloud_id=cloud_id) == quota_pb2.Quota(
            cloud_id=cloud_id,
            metrics=[
                quota_pb2.QuotaMetric(
                    name="smart-captcha.captchas.count",
                    limit=self.DEFAULT_CAPTCHAS_LIMIT,
                    usage=0.0,
                )
            ]
        )

    def assert_quota_failure(self, cloud_id, limit, exception):
        # https://chromium.googlesource.com/external/github.com/grpc/grpc/+/HEAD/examples/python/errors/client.py
        status = rpc_status.from_call(exception)
        assert exception.code() == grpc.StatusCode.RESOURCE_EXHAUSTED
        assert len(status.details) == 1
        detail = status.details[0]

        assert detail.type_url == "type.googleapis.com/yandex.cloud.priv.quota.QuotaFailure"
        failure = quota_pb2.QuotaFailure()
        detail.Unpack(failure)

        assert failure == quota_pb2.QuotaFailure(
            cloud_id=cloud_id,
            violations=[
                quota_pb2.QuotaFailure.Violation(
                    metric=quota_pb2.QuotaMetric(
                        name="smart-captcha.captchas.count",
                        limit=limit,
                        usage=limit,
                    ),
                    required=limit + 1,
                )
            ]
        )

    def test_captchas_quota_exceeded_default(self):
        cloud_id = generate_cloud_id()

        def req():
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=generate_folder_id(cloud_id),
                name="Bum",
                allowed_sites=["example.com", "example.ru"]
            ))

        for i in range(self.DEFAULT_CAPTCHAS_LIMIT):
            req()

        with pytest.raises(grpc.RpcError) as exception_info:
            req()

        self.assert_quota_failure(cloud_id, self.DEFAULT_CAPTCHAS_LIMIT, exception_info.value)

        assert empty_pb2.Empty() == self.captcha_cloud_api.update_quota(
            cloud_id=cloud_id,
            metric=quota_pb2.MetricLimit(
                name="smart-captcha.captchas.count",
                limit=self.DEFAULT_CAPTCHAS_LIMIT + 1,
            )
        )

        req()

        with pytest.raises(grpc.RpcError) as exception_info:
            req()

    def test_captchas_quota_exceeded_1(self):
        cloud_id = generate_cloud_id()
        folder_id = generate_folder_id(cloud_id)
        limit = 1
        assert empty_pb2.Empty() == self.captcha_cloud_api.update_quota(
            cloud_id=cloud_id,
            metric=quota_pb2.MetricLimit(
                name="smart-captcha.captchas.count",
                limit=limit,
            )
        )
        for _ in range(limit):
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=folder_id,
                name="Bum",
                allowed_sites=["example.com", "example.ru"]
            ))
        with pytest.raises(grpc.RpcError) as exception_info:
            self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
                folder_id=folder_id,
                name="Bum",
                allowed_sites=["example.com", "example.ru"]
            ))
        self.assert_quota_failure(cloud_id, limit, exception_info.value)


class TestCaptchaCloudApi2(TestCaptchaCloudApiBase):
    @pytest.mark.parametrize("referer, sitekey, check_substr, cloud_status, expected_status", [
        ("https://example.com",     "{key}", 'window.errorFlag=__errorFlagBackend', "active",  0),
        ("https://sub.example.com", "{key}", 'window.errorFlag=__errorFlagBackend', "active",  0),
        ("https://yandex.ru",       "{key}", 'window.errorFlag="wrong-domain"',     "active",  2),
        ("",                        "{key}", 'window.errorFlag="wrong-domain"',     "active",  2),
        ("https://yandex.ru",       "qwe",   'window.errorFlag="invalid-key"',      "active",  1),
        ("https://example.com",     "qwe",   'window.errorFlag="invalid-key"',      "active",  1),
        ("https://example.com",     "{key}", 'window.errorFlag="suspend"',          "blocked", 0),
    ])
    def test_can_render_captcha(self, referer, sitekey, check_substr, cloud_status, expected_status):
        cloud_id = generate_cloud_id(cloud_status)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        self.unified_agent.pop_event_logs()

        ip = GenRandomIP()
        send_sitekey = sitekey.replace('{key}', new_captcha.client_key)

        resp = self.send_request(
            urllib.request.Request(
                f"{self.get_iframe_url()}?sitekey={send_sitekey}",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Referer": referer,
                },
                data=urllib.parse.urlencode({}).encode(),
                method="GET"
            )
        )
        html_content = resp.read().decode()
        assert check_substr in html_content

        ev = self.unified_agent.wait_event_logs(["TCaptchaIframeShow"])[0]
        assert ev.Event.SiteKey == send_sitekey
        assert ev.Event.Status == expected_status

    @pytest.mark.parametrize("cloud_status, ok_message", [
        ("active", ""),
        ("blocked", "Passed due to suspension."),
    ])
    def test_can_validate(self, cloud_status, ok_message):
        self.unified_agent.billing_log.pop_logs()
        timestamp = int(time.time())

        cloud_id = generate_cloud_id(cloud_status)
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        ip = GenRandomIP()
        spravka = self.get_spravka(ip, new_captcha.client_key)

        resp = self.send_validate_request(spravka, ip, req_id="req-no-auth")
        res = json.loads(resp.read().decode())
        assert res == {
            "status": "failed",
            "message": "Authentication failed. Secret has not provided.",
        }
        assert resp.headers.get("Content-Type") == "application/json"

        resp = self.send_validate_request(spravka, ip, secret="some_unexisten_secret", req_id="req-no-secret")
        res = json.loads(resp.read().decode())
        assert res == {
            "status": "failed",
            "message": "Authentication failed. Invalid secret.",
        }
        assert resp.headers.get("Content-Type") == "application/json"

        resp = self.send_validate_request(spravka, ip, secret=new_captcha.server_key, req_id="req-ok")
        res = json.loads(resp.read().decode())
        assert res == {
            "status": "ok",
            "message": ok_message,
        }
        assert resp.headers.get("Content-Type") == "application/json"

        resp = self.send_validate_request(spravka, ip, secret=new_captcha.server_key, req_id="req-failed")
        res = json.loads(resp.read().decode())
        if cloud_status == "active":
            assert res == {
                "status": "failed",
                "message": "",
            }
        else:
            assert res == {
                "status": "ok",
                "message": ok_message,
            }
        assert resp.headers.get("Content-Type") == "application/json"

        time.sleep(1)
        billing_logs = self.unified_agent.billing_log.get_logs()

        if cloud_status == "active":
            assert len(billing_logs) == 2, str(billing_logs)
            assert billing_logs[0]["id"] == "req-ok"
            assert billing_logs[1]["id"] == "req-failed"
            for log in billing_logs:
                assert log["source_id"]
                assert log["folder_id"] == new_captcha.folder_id
                assert log["resource_id"] == new_captcha.captcha_id
                assert log["version"] == "v1"
                assert log["tags"] == {}
                assert log["schema"] == "smart_captcha.check.requests.v1"
                assert timestamp <= int(log["source_wt"]) <= timestamp + 60
                assert log["usage"] == {
                    "quantity": "1",
                    "start": log["source_wt"],
                    "finish": log["source_wt"],
                    "unit": "unit",
                    "type": "delta",
                }
        else:
            assert len(billing_logs) == 0, str(billing_logs)

    def test_validate_metrics(self):
        self.unified_agent.resource_metrics_raw_log.pop_logs()
        self.unified_agent.resource_metrics_log.pop_logs()

        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        ip = GenRandomIP()

        events_count = 21
        spravkas = [self.get_spravka(ip, new_captcha.client_key) for _ in range(events_count)]
        for spravka in spravkas:
            resp = self.send_validate_request(spravka, ip, secret=new_captcha.server_key, req_id="req-ok")
            res = json.loads(resp.read().decode())
            assert res == {"status": "ok", "message": ""}

            resp = self.send_validate_request(spravka, ip, secret=new_captcha.server_key, req_id="req-failed")
            res = json.loads(resp.read().decode())
            assert res == {"status": "failed", "message": ""}
            time.sleep(0.1)

        time.sleep(1)  # wait accumulate period
        resource_metrics_raw_logs = self.unified_agent.resource_metrics_raw_log.get_logs()
        resource_metrics_logs = self.unified_agent.resource_metrics_log.get_logs()

        assert len(resource_metrics_raw_logs) == events_count * 3, resource_metrics_raw_logs  # check + 2*validate
        success_sum = 0
        failed_sum = 0
        for record in resource_metrics_logs:
            for sensor in record["sensors"]:
                assert sensor["kind"] == "IGAUGE"
                if sensor["labels"]["name"] == "smartcaptcha.captcha.validate.failed_count":
                    failed_sum += sensor["value"]
                elif sensor["labels"]["name"] == "smartcaptcha.captcha.validate.success_count":
                    success_sum += sensor["value"]

                assert sensor["labels"] == {
                    "name": sensor["labels"]["name"],
                    "host": sensor["labels"]["host"],
                    "cloud_id": new_captcha.cloud_id,
                    "folder_id": new_captcha.folder_id,
                    "captcha": new_captcha.captcha_id,
                }
        assert success_sum == events_count and failed_sum == events_count, (success_sum, failed_sum)
        # 21 событие с sleep=0.1 точно не вместится в 2 промежутока, и точно вместится в 4 промежутка
        assert 3 <= len(resource_metrics_logs) <= 4, len(resource_metrics_logs)

    def test_show_check_metrics(self):
        self.unified_agent.resource_metrics_raw_log.pop_logs()

        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        ip = GenRandomIP()

        resp = self.send_request(
            urllib.request.Request(
                f"{self.get_iframe_url()}?sitekey={new_captcha.client_key}",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Referer": "https://example.com",
                },
                data=urllib.parse.urlencode({}).encode(),
                method="GET"
            )
        )
        resp.read().decode()

        self.fury.set_strategy("ButtonStrategy", "fail")
        content = self.get_next_captcha(ip, new_captcha.client_key)
        assert content["status"] == "failed"

        self.fury.set_strategy("Strategy", "fail")
        content = self.get_next_captcha2(ip, content, new_captcha.client_key)
        assert content["status"] == "failed"

        self.fury.set_strategy("Strategy", "success")
        content = self.get_next_captcha2(ip, content, new_captcha.client_key)
        assert content["status"] == "ok"

        self.fury.set_strategy("ButtonStrategy", "success")
        content = self.get_next_captcha(ip, new_captcha.client_key)
        assert content["status"] == "ok"

        time.sleep(1)
        resource_metrics_raw_logs = self.unified_agent.resource_metrics_raw_log.get_logs()

        assert len(resource_metrics_raw_logs) == 5, resource_metrics_raw_logs
        assert len([rec for rec in resource_metrics_raw_logs if rec["metrics"][0]["labels"]["name"] == "smartcaptcha.captcha.checkbox.shows"]) == 1
        assert len([rec for rec in resource_metrics_raw_logs if rec["metrics"][0]["labels"]["name"] == "smartcaptcha.captcha.checkbox.failed_count"]) == 1
        assert len([rec for rec in resource_metrics_raw_logs if rec["metrics"][0]["labels"]["name"] == "smartcaptcha.captcha.advanced.failed_count"]) == 1
        assert len([rec for rec in resource_metrics_raw_logs if rec["metrics"][0]["labels"]["name"] == "smartcaptcha.captcha.checkbox.success_count"]) == 1
        assert len([rec for rec in resource_metrics_raw_logs if rec["metrics"][0]["labels"]["name"] == "smartcaptcha.captcha.advanced.success_count"]) == 1

    def test_check_sitekey_invalid(self):
        ip = GenRandomIP()
        with pytest.raises(Exception) as exception_info:
            self.get_spravka(ip, "shit")
        assert "assert 403 == 200" in str(exception_info.value)

    def test_check_sitekey_valid(self):
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"]
        ))
        ip = GenRandomIP()
        assert self.get_spravka(ip, new_captcha.client_key)
        assert self.get_spravka(ip)

    @pytest.mark.parametrize("method, valid_token, expected_html", [
        ("GET",  False, "<!-- placeholder -->"),
        ("POST", False, "<div class=\"error\">Captcha validation failed</div>"),
        ("POST", True,  "<div class=\"greeting\">Hello, <strong>user</strong>!</div>"),
    ])
    def test_demo_html(self, method, valid_token, expected_html):
        ip = '1.4.3.98'
        client_key = self.captcha_cloud_api.test_captcha.client_key
        resp = self.send_request(
            urllib.request.Request(
                f"http://{self.antirobot.host}/demo",
                headers={
                    "X-Forwarded-For-Y": ip,
                },
                method=method,
                data=urllib.parse.urlencode({
                    "name": "user",
                    "smart-token": self.get_spravka(ip, client_key) if valid_token else '',
                }).encode(),
            ),
        )
        assert resp.getcode() == 200
        assert expected_html in resp.read().decode()

    @pytest.mark.parametrize("complexity, type", [
        (captcha_pb2.CaptchaComplexity.CAPTCHA_COMPLEXITY_UNSPECIFIED, "txt_v1_en"),
        (captcha_pb2.CaptchaComplexity.MEDIUM,                         "txt_v1_en"),
        (captcha_pb2.CaptchaComplexity.EASY,                           "txt_v2_en"),
        (captcha_pb2.CaptchaComplexity.HARD,                           "txt_v3_en"),
    ])
    def test_check_complexity(self, complexity, type):
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            name="Bum",
            allowed_sites=["example.com", "example.ru"],
            complexity=complexity,
        ))
        ip = GenRandomIP()

        self.fury.set_strategy("ButtonStrategy", "fail")

        content = self.get_next_captcha(ip, new_captcha.client_key)
        real_image_url = self.send_request(
            urllib.request.Request(
                content["captcha"]["image"],
                headers={"X-Forwarded-For-Y": ip}
            )
        ).headers["Location"]

        real_type = urllib.parse.parse_qs(urllib.parse.urlparse(real_image_url).query)["type"][0]
        assert real_type == type

    @pytest.mark.parametrize("fake", [
        False,
        True,
    ])
    def test_render_when_api_failed(self, fake):
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            allowed_sites=["example.com"]
        ))

        self.captcha_cloud_api.terminate()

        ip = GenRandomIP()
        send_sitekey = "trust_me_please" if fake else new_captcha.client_key

        resp = self.send_request(
            urllib.request.Request(
                f"{self.get_iframe_url()}?sitekey={send_sitekey}",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Referer": "https://example.com/page",
                },
                data=urllib.parse.urlencode({}).encode(),
                method="GET"
            )
        )
        html_content = resp.read().decode()
        assert "window.errorFlag=__errorFlagBackend" in html_content

        spravka = self.get_spravka(ip, send_sitekey)
        assert spravka

        self.restart_captcha_cloud_api()

        resp = self.send_validate_request(spravka, ip, secret=new_captcha.server_key)
        content = json.loads(resp.read().decode())
        if fake:
            assert content["status"] == "failed", content
        else:
            assert content["status"] == "ok", content

    def test_validate_when_api_failed(self):
        self.captcha_cloud_api.terminate()

        ip = GenRandomIP()
        send_sitekey = "trust_me_please"
        spravka = self.get_spravka(ip, send_sitekey)
        assert spravka

        resp = self.send_validate_request(spravka, ip, secret="xxxxxx")
        content = json.loads(resp.read().decode())
        assert content["status"] == "ok", content

        self.restart_captcha_cloud_api()

    def test_style_json(self):
        cloud_id = generate_cloud_id()
        new_captcha = self.captcha_cloud_api.create_captcha(api_pb2.CreateCaptchaRequest(
            folder_id=generate_folder_id(cloud_id),
            allowed_sites=["example.com"],
            style_json='{"json":1}',
        ))

        ip = GenRandomIP()

        resp = self.send_request(
            urllib.request.Request(
                f"{self.get_iframe_url()}?sitekey={new_captcha.client_key}",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Referer": "https://example.com/page",
                },
                data=urllib.parse.urlencode({}).encode(),
                method="GET"
            )
        )
        html_content = resp.read().decode()
        assert '{"json":1}' in html_content


class TestCaptchaCloudApiTimeout(TestCaptchaCloudApiBase):
    captcha_cloud_api_args = ["--test-sleep-delay", "5000"]

    def test_api_timeout_iframe(self):
        ip = GenRandomIP()
        trust_sitekey = "xxxxxxxxxxxx"

        resp = self.send_request(
            urllib.request.Request(
                f"{self.get_iframe_url()}?sitekey={trust_sitekey}",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Referer": "https://example.com/page",
                },
                data=urllib.parse.urlencode({}).encode(),
                method="GET"
            )
        )
        html_content = resp.read().decode()
        assert "__errorFlagBackend" in html_content  # iframe должен отобразиться без ошибок, потому что API не ответило

    def test_api_timeout_check(self):
        ip = GenRandomIP()
        trust_sitekey = "xxxxxxxxxxxx"

        self.unified_agent.pop_event_logs()

        spravka = self.get_spravka(ip, trust_sitekey)
        assert spravka

        events = self.unified_agent.wait_event_logs(
            lambda e: e.EventType == "TRequestGeneralMessage" and "GetSettingsByClientKey error" in e.Event.Message,
            max_count=100
        )
        assert len(events) >= 1
