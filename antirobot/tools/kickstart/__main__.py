from pathlib import Path
import argparse
import asyncio
import contextlib
import json
import os
import random
import re
import signal
import socket
import sys
import tempfile
import time

import sandbox.common.rest

import yatest

RULE_ID_RE = re.compile("^rule_id=(\\d+); *")


async def main():
    args = parse_args()

    arcadia_root = Path(args.arcadia_root).absolute()
    yatest.common = YatestCommon(arcadia_root)
    yatest.common.network.PortManager.reserve_port(args.balancer_port)

    data_root = Path(args.data_root).absolute()
    cbb_path = Path(args.load_cbb).absolute()

    with ResourceDir.setup(data_root) as resource_dir:
        eprint("Updating test data...")
        await resource_dir.update({
            "data": args.data_id,
        })

        os.chdir(data_root)

        from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
        balancer_mock = None

        try:
            eprint("Starting Antirobot...")
            AntirobotTestSuite.setup_class()

            balancer_mock_path = arcadia_root / "antirobot/tools/balancer_mock/balancer_mock"
            optional_balancer_mock_args = []

            if args.balancer_service:
                optional_balancer_mock_args += ["--antirobot-service", args.balancer_service]

            antirobot_port = AntirobotTestSuite.antirobot.dump_cfg()["Port"]

            balancer_mock = await asyncio.create_subprocess_exec(
                balancer_mock_path,
                "--port", str(args.balancer_port),
                "--antirobot", f"localhost:{antirobot_port}",
                *optional_balancer_mock_args,
            )

            if args.load_cbb:
                eprint("Loading CBB...")
                load_cbb(AntirobotTestSuite, cbb_path)

            eprint("Kickstart successful.")
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            eprint("Shutting down...")
        finally:
            if balancer_mock is not None:
                balancer_mock.terminate()
                await balancer_mock.wait()

            eprint("Tearing down Antirobot...")
            AntirobotTestSuite.exit()


def parse_args():
    parser = argparse.ArgumentParser(
        description="launch Antirobot with test data",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--arcadia-root",
        required=True,
        help="path to Arcadia root",
    )

    parser.add_argument(
        "--data-root",
        required=True,
        help="path to root directory for test data",
    )

    parser.add_argument(
        "--data-id",
        type=int,
        required=True,
        help="Antirobot data sandbox id",
    )

    parser.add_argument(
        "--balancer-port",
        type=int,
        required=True,
        help="balancer_mock port",
    )

    parser.add_argument(
        "--balancer-service",
        help="service type to pass to Antirobot",
    )

    parser.add_argument(
        "--load-cbb",
        help="path to CBB dump made by antirobot/tools/dump_cbb",
    )

    return parser.parse_args()


class YatestCommon:
    def __init__(self, arcadia_root):
        self.arcadia_root = arcadia_root

    def build_path(self, path):
        return str(self.arcadia_root / path)

    def source_path(self, path):
        return self.build_path(path)

    def get_param(self, key, default=None):
        return default

    class network:
        class PortManager:
            __reserved_ports = set()

            def __init__(self):
                self.used_ports = set(self.__reserved_ports)

            def __enter__(self):
                return self

            def __exit__(self, exc_type, exc_value, traceback):
                return None

            @classmethod
            def reserve_port(cls, port):
                cls.__reserved_ports.add(port)

            def get_port(self):
                ports = list(range(1024, 65535 + 1))
                random.shuffle(ports)

                for port in ports:
                    if port in self.used_ports:
                        continue

                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                        if sock.connect_ex(("127.0.0.1", port)) != 0:
                            self.used_ports.add(port)
                            return port

                raise Exception("failed to find a port")


class ResourceDir:
    __name_to_ident_path = "kickstart_resource_ids.json"

    def __init__(self, root, name_to_ident):
        self.sandbox_client = sandbox.common.rest.Client()
        self.root = root
        self.name_to_ident = name_to_ident

    async def update(self, name_to_ident):
        with contextlib.ExitStack() as stack:
            tmp_dir = None
            download_tasks = []

            for name, ident in name_to_ident.items():
                if name in self.name_to_ident and self.name_to_ident[name] == ident:
                    continue

                if tmp_dir is None:
                    tmp_dir = Path(stack.enter_context(tempfile.TemporaryDirectory()))

                download_tasks.append(self.__update_one(tmp_dir, name, ident))

            await asyncio.gather(*download_tasks)

    async def __update_one(self, tmp_dir, name, ident):
        process = await asyncio.create_subprocess_exec("ya", "download", f"sbr://{ident}", cwd=tmp_dir)
        await process.wait()

        attrs = self.sandbox_client.resource[ident].read()
        path = attrs.get("file_name") or ident

        with SignalDelayer():
            full_path = self.root / path

            if full_path.exists():
                full_path.rename(tmp_dir / f"old_{ident}")

            (tmp_dir / str(ident)).rename(full_path)
            self.name_to_ident[name] = ident

    @classmethod
    @contextlib.contextmanager
    def setup(cls, root):
        root = Path(root)
        root.mkdir(exist_ok=True)

        try:
            with open(root / cls.__name_to_ident_path) as name_to_ident_file:
                name_to_ident = json.load(name_to_ident_file)
        except FileNotFoundError:
            name_to_ident = {}

        resource_dir = cls(root, name_to_ident)

        try:
            yield resource_dir
        finally:
            resource_dir.__dump_ids()

    def __dump_ids(self):
        with open(self.root / self.__name_to_ident_path, "w") as name_to_ident_file:
            json.dump(self.name_to_ident, name_to_ident_file)


class SignalDelayer:
    __sigs = (signal.SIGINT, signal.SIGTERM)

    def __enter__(self):
        self.__signals = []
        self.__sig_to_handler = {}

        for sig in self.__sigs:
            self.__sig_to_handler[sig] = signal.signal(sig, self.__handler)

        return self

    def __handler(self, sig, frame):
        self.__signals.append(sig, frame)

    def __exit__(self, exc_type, exc_value, traceback):
        for sig, handler in self.__sig_to_handler.items():
            signal.signal(sig, handler)

        for sig, frame in self.__signals:
            self.__sig_to_handler[sig](sig, frame)


def load_cbb(suite, path):
    with open(path) as cbb_file:
        cbb_data = json.load(cbb_file)

    for ident, rules in cbb_data["txt"].items():
        for rule in rules:
            rule_id_match = RULE_ID_RE.match(rule)
            if rule_id_match:
                suite.cbb.set_range("add", {
                    "flag": ident,
                    "rule_id": rule_id_match.group(1),
                    "range_txt": rule[rule_id_match.end():],
                })

    for ident, rules in cbb_data["re"].items():
        for rule in rules:
            suite.cbb.set_range("add", {"flag": ident, "range_re": rule})


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


if __name__ == "__main__":
    asyncio.run(main())
