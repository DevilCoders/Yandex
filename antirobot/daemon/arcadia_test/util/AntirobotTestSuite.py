import collections
import contextlib
import copy
import glob
import json
import os
from pathlib import Path
import random
import re
import shlex
import subprocess
import tarfile
import tempfile
import urllib.parse
import urllib.request

import cityhash

import yatest.common
import yatest.common.network


from antirobot.daemon.arcadia_test import util
from antirobot.daemon.arcadia_test.util import (
    asserts,
    daemon,
    ProtoListIO,
)
from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess
from antirobot.daemon.arcadia_test.util.cbb_mock import Cbb
from antirobot.daemon.arcadia_test.util.fury_mock import Fury
from antirobot.daemon.arcadia_test.util.resource_manager_mock import ResourceManager
from antirobot.daemon.arcadia_test.util.access_service_mock import AccessService
from antirobot.daemon.arcadia_test.util.captcha_mock import Captcha
from antirobot.daemon.arcadia_test.util.wizard_mock import Wizard
from antirobot.daemon.arcadia_test.util.unified_agent_mock import UnifiedAgent
from antirobot.daemon.arcadia_test.util.discovery_mock import Discovery
from antirobot.daemon.arcadia_test.util.ydb_mock import Ydb
from antirobot.daemon.arcadia_test.util.captcha_cloud_api_mock import CaptchaCloudApi
from antirobot.idl import factors_pb2


TEST_DATA_ROOT = Path("antirobot").absolute()

_CONFIG_LINE_RE = re.compile("^( *)([a-zA-Z0-9_]+)( *= *)(.*)$")


class Antirobot(NetworkSubprocess):
    def __init__(self, process, options):
        super().__init__(process, options["Port"])
        self.options = options.copy()
        self.admin_port = options["AdminServerPort"]
        self.unistat_port = options["UnistatServerPort"]
        self.process_port = options["ProcessServerPort"]
        self.url_opener = urllib.request.build_opener(daemon.NoRedirection)

    @classmethod
    @contextlib.contextmanager
    def make(
        cls,
        options,
        parent_context=None,
        args=[],
        bin_folder=None,
        global_config=None,
        captcha=None,
        fury=None,
        fury_preprod=None,
        cbb=None,
        unified_agent=None,
        wizard=None,
        discovery=None,
        mute=None,
        rr_record=None,
        rr_params=None,
        experiments_config=None,
    ):
        if parent_context is not None:
            args = args or parent_context.args
            global_config = global_config or parent_context.global_config
            captcha = captcha or parent_context.captcha
            fury = fury or parent_context.fury
            fury_preprod = fury_preprod or parent_context.fury_preprod
            cbb = cbb or parent_context.cbb
            unified_agent = unified_agent or parent_context.unified_agent
            wizard = wizard or parent_context.wizard
            discovery = discovery or parent_context.discovery
            experiments_config = experiments_config or parent_context.experiments_config

            if mute is None:
                mute = parent_context.mute

            if rr_record is None:
                rr_record = parent_context.rr_record

            if rr_params is None:
                rr_params = parent_context.rr_params

        if bin_folder is None:
            bin_path = yatest.common.build_path("antirobot/daemon/antirobot_daemon")
            gencfg_path = yatest.common.build_path("antirobot/scripts/gencfg/antirobot_gencfg")
            service_config_path = yatest.common.source_path("antirobot/config/service_config.json")
            service_ident_path = yatest.common.source_path("antirobot/config/service_identifier.json")
            global_config_path = yatest.common.source_path("antirobot/config/global_config.json")
        else:
            bin_path = os.path.join(bin_folder, "antirobot_daemon")
            gencfg_path = os.path.join(bin_folder, "antirobot_gencfg")
            service_config_path = os.path.join(bin_folder, "arcadia_config/service_config.json")
            service_ident_path = os.path.join(bin_folder, "arcadia_config/service_identifier.json")
            global_config_path = os.path.join(bin_folder, "arcadia_config/global_config.json")

        default_options = {
            "BaseDir": str(TEST_DATA_ROOT),
            "CbbEnabled": 0,
        }

        options = {**default_options, **options}

        if captcha is not None:
            options["CaptchaApiHost"] = captcha.host

        if fury is not None:
            options["FuryEnabled"] = "1"
            options["FuryHost"] = fury.host

        if fury_preprod is not None:
            options["FuryPreprodEnabled"] = "1"
            options["FuryPreprodHost"] = fury_preprod.host

        if cbb is not None:
            options.update(
                CbbApiHost=cbb.host,
                CbbEnabled=1,
            )

        if unified_agent is not None:
            options["UnifiedAgentUri"] = unified_agent.host

        if wizard is not None:
            options["LocalWizard"] = wizard.host
            options["RemoteWizards"] = wizard.host
            options["WizardPort"] = wizard.port

        if discovery is not None:
            options["DiscoveryHost"] = "localhost"
            options["DiscoveryPort"] = discovery.port

        options["Page404Host"] = "localhost"

        with \
                contextlib.ExitStack() as stack, \
                tempfile.NamedTemporaryFile("w") as config_file, \
                tempfile.NamedTemporaryFile("w") as service_config_file, \
                tempfile.NamedTemporaryFile("w") as experiments_config_file, \
                tempfile.TemporaryDirectory() as runtime_dir:
            runtime_dir_abs = Path(runtime_dir).absolute()

            if global_config is not None:
                global_config_file = stack.enter_context(tempfile.NamedTemporaryFile("w"))

                with open(global_config_path) as f:
                    last_visits = json.load(f).get('last_visits')

                default_global_config = {
                    "rules": [],
                    "mark_rules": [],
                    "dictionaries_meta": [],
                    "dictionaries": [],
                    "last_visits": last_visits,
                }

                if 'last_visits' in global_config:
                    global_config['last_visits'].extend(last_visits)

                json.dump({**default_global_config, **global_config}, global_config_file)
                global_config_file.flush()

                global_config_path = global_config_file.name

            def make_factor_records(test_data, key_type):
                result = []
                header = ''
                if key_type == "uid":
                    header = "TUidRecord"
                    for k in test_data:
                        proto = factors_pb2.TUidRecord
                        rec = proto()
                        rec.Key = k
                        result.append(rec)
                else:
                    for k, v in test_data:
                        if key_type in ["string", "subnet"]:
                            proto = factors_pb2.TFloatRecord
                            rec = proto()
                            rec.Key = k
                            rec.Value = v
                            result.append(rec)
                            header = "TFloatRecord"
                        elif key_type == "cityhash64":
                            proto = factors_pb2.TCityHash64FloatRecord
                            rec = proto()
                            rec.Key = cityhash.hash64(k.encode())
                            rec.Value = v
                            result.append(rec)
                            header = "TCityHash64FloatRecord"
                        elif key_type == "mini_geobase":
                            proto = factors_pb2.TFixed64Record
                            rec = proto()
                            rec.Key = k
                            rec.Value = v
                            result.append(rec)
                            header = "TFixed64Record"
                        elif key_type == "jws_stats":
                            proto = factors_pb2.TMarketJwsStatesStats
                            rec = proto()
                            rec.Key = k
                            for name, value in v.items():
                                setattr(rec, name, value)
                            result.append(rec)
                            header = "TMarketJwsStatesStats"
                        elif key_type == "market_stats":
                            proto = factors_pb2.TMarketStats
                            rec = proto()
                            rec.Key = cityhash.hash64(k.encode())
                            for name, value in v.items():
                                setattr(rec, name, value)
                            result.append(rec)
                            header = "TMarketStats"
                        else:
                            raise Exception(f"Unknown key_type {key_type}")

                assert header
                recH = factors_pb2.THeader()
                recH.Header = header
                recH.Num = len(result)
                return (proto, result, recH)

            dictionaries_dir = Path(options.get("DictionariesDir", str(TEST_DATA_ROOT / "data" / "dictionaries")))
            dictionaries_dir.mkdir(parents=True, exist_ok=True)
            with open(global_config_path) as inp:
                json_data = json.load(inp)
                dicts = json_data.get("dictionaries", [])

                for data in json_data.get("dictionaries_meta", []):
                    if data["name"] in dicts and "test_data" in data:
                        dict_path = dictionaries_dir / data["name"]
                        proto, test_records, header = make_factor_records(data["test_data"], data["key_type"])
                        ProtoListIO(proto).write(str(dict_path), test_records, header)

            default_experiments_config = {
                "processor_experiments": {
                    "catboostWebExp2.info": {
                        "threshold": 2.0,
                        "probability": {
                            "web": 1.0,
                            "classified": 1.0,
                            "default": 0.0
                        }
                    }
                },
                "cacher_experiments": {
                    "cacher_catboost_v3.info": {
                        "threshold": 2.0,
                        "probability": {
                            "web": 1.0,
                            "classified": 1.0,
                            "default": 0.0
                        }
                    }
                }
            }

            json.dump(experiments_config or default_experiments_config, experiments_config_file)
            experiments_config_file.flush()

            options.update(
                RuntimeDataDir=runtime_dir_abs,
                LogsDir=runtime_dir_abs,
                GlobalJsonConfFilePath=Path(global_config_path).absolute(),
                JsonConfFilePath=Path(service_config_file.name).absolute(),
                JsonServiceRegExpFilePath=Path(service_ident_path).absolute(),
                GenCfgPath=Path(gencfg_path).absolute(),
                ExperimentsConfigFilePath=Path(experiments_config_file.name).absolute(),
            )
            config = cls.__config(options)
            config_file.write(config)
            config_file.flush()

            service_config = cls.__service_config(options, service_config_path)
            json.dump(service_config, service_config_file)
            service_config_file.flush()

            if mute:
                redir = subprocess.DEVNULL
            else:
                redir = None

            argv = [
                bin_path,
                "-c", config_file.name,
                "--no-tvm",
                *map(str, args),
            ]

            if rr_record:
                argv = ["rr", "record"] + rr_params + argv

            with subprocess.Popen(argv, stdout=redir, stderr=redir) as process:
                try:
                    yield cls(process, options)
                finally:
                    process.terminate()

    @classmethod
    def __config(cls, options):
        base = cls.__base_config(options.get('GenCfgPath'))
        lines = []

        for line in base:
            if isinstance(line, str):
                lines.append(line)
            else:
                left, key, middle, value = line
                new_value = options.get(key, value)
                lines.append(f"{left}{key}{middle}{new_value}")

        result = "\n".join(lines)
        for forbidden_host in (".yandex-team.ru", ".yandex.net", ".yandex.ru"):
            if forbidden_host in result:
                before, after = result.split(forbidden_host, 1)
                assert False, before[-20:] + forbidden_host + after[:20]
        return result

    @classmethod
    def __base_config(cls, gencfg_path):
        base_config = []
        gencfg_output = subprocess.check_output([gencfg_path, "--local"]).decode()
        gencfg_output = gencfg_output.replace('{0.9:txt_v1,0.1:txt_v1_teacher}', 'txt_v1')  # TODO: temporary hack

        for line in gencfg_output.splitlines():
            match = _CONFIG_LINE_RE.match(line)

            if match is None:
                base_config.append(line)
            else:
                base_config.append(match.groups())

        return base_config

    @classmethod
    def __service_config(cls, options, base_service_config_path):
        with open(base_service_config_path) as file:
            config = json.load(file)

        for service in config:
            for key, value in options.items():
                key_parts = key.split("@", 1)

                if len(key_parts) == 2:
                    if key_parts[1] == service["service"]:
                        service[key_parts[0]] = value
                else:
                    if key_parts[0] in service:
                        service[key_parts[0]] = value

        return config

    def terminate(self):
        self.process.terminate()
        self.process.wait()

    def is_alive(self):
        return self.process.poll() is None

    def finished_ok(self):
        return self.process.poll() == 0

    def send_request(self, request, port=None):
        if port is None:
            if isinstance(request, util.ProcessReq):
                port = self.process_port
            else:
                port = self.port

        if isinstance(request, (util.Fullreq, util.ProcessReq)):
            request = request.GetRequestForHost(f"http://localhost:{port}")
        else:
            request = daemon.AddDaemonHost(request, f"localhost:{port}")

        return self.url_opener.open(request)

    def send_fullreq(self, *args, **kwargs):
        return self.send_request(util.Fullreq(*args, **kwargs))

    def ping_search(self, ip=None, query="hello", headers={}):
        if ip is None:
            ip = util.GenRandomIP()

        return self.send_fullreq(
            f"http://yandex.ru/search?text={query}",
            headers={"X-Forwarded-For-Y": ip, **headers},
        )

    def dump_cfg(self, service=""):
        resp = self.send_request(f"/admin?action=dumpcfg&service={service}", self.admin_port)
        config_str = resp.read().decode()
        assert resp.getcode() == 200, (resp.getcode(), config_str)

        config = {}

        for line in config_str.strip().splitlines():
            key, value = (token.strip() for token in line.split("=", 1))
            config[key] = value

        return config

    def is_banned(self, ip, path="yandsearch", headers=None):
        headers = (headers or {}).copy()
        headers["X-Forwarded-For-Y"] = ip

        response = self.send_fullreq(
            f"http://yandex.ru/{path}?text=check_is_banned",
            headers=headers,
        )

        return util.IsCaptchaRedirect(response)

    def ban(self, ip, service="yandsearch", check=True, headers=None):
        headers = (headers or {}).copy()
        headers["X-Forwarded-For-Y"] = ip

        self.send_fullreq(
            f"http://yandex.ru/{service}?text=e48a2b93de1740f48f6de0d45dc4192a",
            headers=headers,
        )

        if check:
            asserts.AssertEventuallyTrue(
                lambda: self.is_banned(ip, path=service, headers=headers),
                secondsBetweenCalls=0.25,
            )

    def block(self, ip, duration=30):
        self.send_request(f"/admin?action=block&ip={ip}&duration={duration}s")

    def is_blocked(self, ip):
        response = self.send_fullreq(
            "http://yandex.ru/yandsearch?text=check_is_blocked",
            headers={
                "X-Forwarded-For-Y": ip,
            },
        )

        return util.IsBlockedResponse(response)

    def get_stats(self, port=None):
        port = port or self.unistat_port
        response = self.send_request("/unistats", port)
        return dict(json.load(response))

    def get_metric(self, name, port=None):
        return next(
            value for key, value in self.get_stats(port).items()
            if key == name
        )

    def query_metric(self, name, port=None, aggr=lambda x, y: x + y, stats=None, **kwargs):
        if stats is None:
            stats = self.get_stats()

        aggr_value = None

        for key, value in stats.items():
            parts = key.split(";")
            param_parts, key_name = parts[:-1], parts[-1]
            params = (part.split("=", 1) for part in param_parts)

            if key_name == name and all(
                param_key not in kwargs or kwargs[param_key] == param_value
                for param_key, param_value in params
            ):
                if aggr_value is None:
                    aggr_value = value
                else:
                    aggr_value = aggr(aggr_value, value)

        return aggr_value

    def queues_are_empty(self):
        stats = self.get_stats()

        processingThreadTotal = int(stats['processing_queue.hlmq_threads_total_ahhh'])
        processingThreadFree = int(stats['processing_queue.hlmq_threads_free_ahhh'])
        processingQueueSize = int(stats['processing_queue.hlmq_queue_size_ahhh'])
        processorResponseApplyQueueThreadTotal = int(stats['processor_response_apply_queue.hlmq_threads_total_ahhh'])
        processorResponseApplyQueueThreadFree = int(stats['processor_response_apply_queue.hlmq_threads_free_ahhh'])
        processorResponseApplyQueueQueueSize = int(stats['processor_response_apply_queue.hlmq_queue_size_ahhh'])
        return processingQueueSize == 0 and processingThreadTotal == processingThreadFree and \
               processorResponseApplyQueueQueueSize == 0 and \
               processorResponseApplyQueueThreadTotal == processorResponseApplyQueueThreadFree

    def amnesty(self, service=None):
        url = "/admin?action=amnesty"

        if service is not None:
            url += f"&service={service}"

        self.send_request(url)

    def reload_data(self):
        self.send_request("/admin?action=reloaddata")

    def all_ports(self):
        port_keys = ["Port", "AdminServerPort", "UnistatServerPort"]

        if not self.options.get("AsCaptchaApiService"):
            port_keys.append("ProcessServerPort")

        return [self.options[key] for key in port_keys]


class AntirobotTestSuite:
    args = []
    options = {}
    global_config = None
    captcha_args = []
    fury_args = []
    fury_preprod_args = []
    resource_manager_args = []
    access_service_args = []
    cbb_args = []
    unified_agent_args = []
    wizard_args = []
    discovery_args = None
    ydb_args = None
    captcha_cloud_api_args = None
    num_antirobots = 1
    num_old_antirobots = 0

    captcha = None
    fury = None
    fury_preprod = None
    access_service = None
    cbb = None
    unified_agent = None
    wizard = None
    discovery = None
    ydb = None
    captcha_cloud_api = None
    antirobot = None
    antirobots = []
    experiments_config = None

    __captcha_dirty = False

    __method_run_counter = collections.Counter()

    @classmethod
    def setup_class(cls):
        cls.__stack = contextlib.ExitStack().__enter__()

        try:
            cls.mute = yatest.common.get_param("mute", "0") == "1"
            cls.rr_record = yatest.common.get_param("rr_record", "0") == "1"
            cls.rr_params = shlex.split(yatest.common.get_param("rr_params", ""))

            if len(cls.rr_params) > 0:
                cls.rr_record = True

            cls.setup_lsan()

            cls.__port_manager = cls.enter(
                yatest.common.network.PortManager(),
            )

            if cls.cbb_args is not None:
                assert len(cls.cbb_args) == 0
                cls.cbb = cls.enter(
                    Cbb(cls.get_port(), mute=True),
                )

            if cls.unified_agent_args is not None:
                assert len(cls.unified_agent_args) == 0
                cls.unified_agent = cls.enter(
                    UnifiedAgent(cls.get_port(), mute=cls.mute)
                )

            if cls.wizard_args is not None:
                assert len(cls.wizard_args) == 0
                cls.wizard = cls.enter(
                    Wizard(cls.get_port(), mute=cls.mute),
                )

            if cls.discovery_args is not None:
                assert len(cls.discovery_args) == 0
                cls.discovery = cls.enter(
                    Discovery(cls.get_port(), cls.get_port(), mute=cls.mute),
                )

            if cls.resource_manager_args is not None:
                cls.resource_manager = cls.enter(
                    ResourceManager(cls.get_port(), cls.resource_manager_args),
                )

            if cls.access_service_args is not None:
                cls.access_service = cls.enter(
                    AccessService(cls.get_port(), cls.access_service_args),
                )

            if cls.ydb_args is not None:
                assert cls.ydb_args == []
                cls.options["YdbEndpoint"] = os.getenv('YDB_ENDPOINT')
                cls.options["YdbDatabase"] = os.getenv('YDB_DATABASE')
                cls.ydb = Ydb(cls.options["YdbEndpoint"], cls.options["YdbDatabase"])
            else:
                cls.options["YdbEndpoint"] = "none"
                cls.options["YdbDatabase"] = "none"

            if cls.captcha_cloud_api_args is not None:
                assert cls.ydb
                cls.restart_captcha_cloud_api()
                cls.options["CloudCaptchaApiEndpoint"] = cls.captcha_cloud_api.host
                os.environ["SMARTCAPTCHA_SECRET_KEY"] = cls.captcha_cloud_api.test_captcha.server_key
            else:
                cls.options["CloudCaptchaApiEndpoint"] = "none"

            cls.restart_captcha(cls.captcha_args, need_wait=False)
            cls.restart_fury(cls.fury_args, need_wait=False)
            cls.restart_fury_preprod(cls.fury_preprod_args, need_wait=False)

            if hasattr(cls, "setup_subclass_before_antirobots"):
                cls.setup_subclass_before_antirobots()

            if cls.discovery is not None:
                cls.discovery.wait()

            cls.antirobots = cls.enter(cls.start_antirobots(
                cls.options,
                args=cls.args,
                global_config=cls.global_config,
                num_antirobots=cls.num_antirobots,
                num_old_antirobots=cls.num_old_antirobots
            ))

            if cls.num_antirobots > 0:
                cls.antirobot = cls.antirobots[0]

            if hasattr(cls, "setup_subclass"):
                cls.setup_subclass()
            for mock_name in ("captcha", "fury", "fury_preprod", "wizard", "cbb", "unified_agent", "resource_manager", "access_service"):
                mock = getattr(cls, mock_name)

                if mock is not None:
                    mock.wait()
        except:
            cls.__stack.__exit__(None, None, None)
            raise

    @classmethod
    def setup_lsan(cls):
        lsan_blacklist_file = cls.enter(tempfile.NamedTemporaryFile("w"))
        lsan_blacklist_file.write("leak:kchashdb.h\n")
        lsan_blacklist_file.flush()

        lsan_options = f"suppressions={lsan_blacklist_file.name}"
        if os.environ.get("LSAN_OPTIONS"):
            os.environ["LSAN_OPTIONS"] += "," + lsan_options
        else:
            os.environ["LSAN_OPTIONS"] = lsan_options

    @classmethod
    @contextlib.contextmanager
    def start_antirobots(
        cls,
        options,
        args=[],
        global_config=None,
        num_antirobots=1,
        num_old_antirobots=0,
        mute=None,
        rr_record=None,
        rr_params=None,
        wait=True,
    ):
        if mute is None:
            mute = cls.mute

        if rr_record is None:
            rr_record = cls.rr_record

        if rr_params is None:
            rr_params = cls.rr_params

        options = copy.deepcopy(options)
        ports = cls.get_ports(num_antirobots + num_old_antirobots)
        admin_ports = cls.get_ports(num_antirobots + num_old_antirobots)
        unistat_ports = cls.get_ports(num_antirobots + num_old_antirobots)
        process_ports = cls.get_ports(num_antirobots + num_old_antirobots)
        old_antirobot_bin_path = './antirobot_prev/antirobot_daemon'
        old_gencfg_bin_path = './antirobot_prev/antirobot_gencfg'

        if num_old_antirobots > 0 and not os.path.exists(old_antirobot_bin_path):
            archive_names = glob.glob("yandex-antirobot-bundle.*.tar.gz")
            assert len(archive_names) == 1
            with tarfile.open(archive_names[0], "r:gz") as archive:
                archive.extractall(path='./antirobot_prev')
            assert os.path.exists(old_antirobot_bin_path)
            assert os.path.exists(old_gencfg_bin_path)

        bin_folders = [None] * num_antirobots + ["./antirobot_prev"] * num_old_antirobots

        if "AllDaemons" not in options:
            options["AllDaemons"] = " ".join(sorted(
                f"localhost:{port}"
                for port in process_ports
            ))

        with contextlib.ExitStack() as stack:
            antirobots = []
            experiments_config = {
                "processor_experiments": {
                    "catboostWebExp2.info": {
                        "threshold": 2.0,
                        "probability": {
                            "web": 1.0,
                            "classified": 1.0,
                            "default": 0.0
                        }
                    }
                },
                "cacher_experiments": {
                    "cacher_catboost_v3.info": {
                        "threshold": 2.0,
                        "probability": {
                            "web": 1.0,
                            "classified": 1.0,
                            "default": 0.0
                        }
                    }
                }
            }
            for port, admin_port, unistat_port, process_port, bin_folder in zip(
                    ports, admin_ports, unistat_ports, process_ports, bin_folders
            ):
                options.update(
                    Port=port,
                    AdminServerPort=admin_port,
                    UnistatServerPort=unistat_port,
                    ProcessServerPort=process_port,
                )

                antirobot = Antirobot.make(
                    options,
                    args=args,
                    global_config=global_config,
                    captcha=cls.captcha,
                    fury=cls.fury,
                    fury_preprod=cls.fury_preprod,
                    cbb=cls.cbb,
                    unified_agent=cls.unified_agent,
                    wizard=cls.wizard,
                    discovery=cls.discovery,
                    bin_folder=bin_folder,
                    mute=mute,
                    rr_record=rr_record,
                    rr_params=rr_params,
                    experiments_config=experiments_config,
                )

                antirobots.append(stack.enter_context(antirobot))

            if wait:
                for antirobot in antirobots:
                    antirobot.wait()

            yield antirobots

    @classmethod
    def restart_captcha(cls, args, need_wait=True):
        cls.restart_mock('captcha', args, Captcha, need_wait=need_wait)

    @classmethod
    def restart_fury(cls, args, need_wait=True):
        cls.restart_mock('fury', args, Fury, need_wait=need_wait)

    @classmethod
    def restart_fury_preprod(cls, args, need_wait=True):
        cls.restart_mock('fury_preprod', args, Fury, need_wait=need_wait)

    @classmethod
    def restart_captcha_cloud_api(cls, args=None, need_wait=True):
        if args is None:
            args = cls.captcha_cloud_api_args + [
                "--ydb-endpoint", cls.ydb.endpoint,
                "--ydb-database", cls.ydb.database,
                "--resource-manager-endpoint", f":{cls.resource_manager.port}",
                "--iam-access-service-endpoint", f":{cls.access_service.port}",
                "--unified-agent-uri", f":{cls.unified_agent.port}"
            ]
        cls.restart_mock('captcha_cloud_api', args, CaptchaCloudApi, need_wait=need_wait)

    @classmethod
    def restart_mock(cls, name, args, constructor, need_wait=True):
        mock = getattr(cls, name, None)
        if mock:
            port = mock.port
            mock.terminate()
        else:
            port = cls.get_port()

        setattr(cls, name, cls.enter(constructor(
            port,
            args,
            mute=cls.mute,
        )))

        if need_wait:
            getattr(cls, name).wait()

    @classmethod
    def get_ports(cls, n):
        return [cls.get_port() for _ in range(n)]

    @classmethod
    def exit(cls):
        cls.__stack.__exit__(None, None, None)

    @classmethod
    def teardown_class(cls):
        cls.exit()

        # TODO(rzhikharevich): For some reason antirobot returns a non-zero status code under rr.
        if not cls.rr_record:
            assert all(antirobot.finished_ok() for antirobot in cls.antirobots)

    def setup_method(self, method):
        test_key = f"{type(self).__name__}.{method.__name__}"
        self.__method_run_counter.update((test_key,))
        random.seed(hash(f"{test_key}[{self.__method_run_counter[test_key]}]"))

        if self.__captcha_dirty:
            self.reset_captcha(self.captcha_args)
            self.__captcha_dirty = False

        if util.service_available(self.cbb.port):
            self.cbb.clear()

        if self.discovery is not None and util.service_available(self.discovery.port):
            self.discovery.clear()

    def teardown_method(self):
        assert all(antirobot.is_alive() for antirobot in self.antirobots)

    @classmethod
    def enter(cls, ctxmgr):
        return cls.__stack.enter_context(ctxmgr)

    @classmethod
    def send_request(cls, *args, **kwargs):
        return cls.antirobot.send_request(*args, **kwargs)

    @classmethod
    def send_fullreq(cls, *args, **kwargs):
        return cls.antirobot.send_fullreq(*args, **kwargs)

    @classmethod
    def reset_captcha(cls, args):
        cls.__captcha_dirty = True
        cls.captcha.terminate()
        cls.captcha = cls.enter(
            Captcha(cls.captcha.port, args, mute=cls.mute),
        )

    @classmethod
    def keys_path(cls):
        return TEST_DATA_ROOT / "data" / "keys"

    @classmethod
    def spravka_data_key(cls):
        return open(TEST_DATA_ROOT / "data" / "spravka_data_key.txt", "r").read().strip()

    @classmethod
    def get_port(cls):
        return cls.__port_manager.get_port()

    @classmethod
    def get_last_event_in_daemon_logs(cls):
        return cls.unified_agent.get_last_event_in_daemon_logs()

    @classmethod
    def req2info(cls, request, rawreq=False):
        bin_path = yatest.common.build_path("antirobot/tools/req2info/req2info")
        args = [bin_path, "--host", cls.antirobot.host]
        if rawreq:
            args += ["--rawreq"]
            program_input = request.encode()
        else:
            args += [request]
            program_input = b''

        p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (stdout, err) = p.communicate(input=program_input)
        assert p.returncode == 0, f"Return code: {p.returncode == 0}; stderr: {err};"
        return stdout.decode()

    @classmethod
    def get_valid_spravka(cls, domain='yandex.ru'):
        daemon = cls.antirobot
        key = cls.spravka_data_key()
        valid_spravka = daemon.send_request(f"/admin?action=getspravka&domain={domain}", daemon.admin_port) \
            .read() \
            .decode() \
            .strip()
        asserts.AssertSpravkaValid(cls, valid_spravka, domain=domain, key=key)
        return valid_spravka
