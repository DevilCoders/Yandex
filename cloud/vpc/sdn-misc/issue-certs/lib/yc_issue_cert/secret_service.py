"""Syncs SecretData created earlier into Secret Service
using yc-secret-cli (NOTE: user yc-secret-cli is used here,
so only profile can be selected)."""

from collections import defaultdict
from datetime import datetime
import json
import logging
import os
import subprocess
from typing import List

from yc_issue_cert.cluster import Cluster
from yc_issue_cert.config import SecretServiceConfig
from yc_issue_cert.secrets import SecretData
from yc_issue_cert.utils import InternalError

import yaml

log = logging.getLogger("ss")


EXPIRE_DATEFMT_IN = "%Y-%m-%dT%H:%M:%SZ"
EXPIRE_DATEFMT_OUT = "%Y-%m-%d"


class SecretServiceCli:
    def __init__(self, config: SecretServiceConfig, profile: str):
        self.config = config
        self.profile = profile

    def gen(self, *cmdargs):
        return ["yc-secret-cli", "--profile", self.profile, *cmdargs]

    def run(self, *cmdargs):
        cmdargs = self.gen(*cmdargs)
        log.debug("Running %s", " ".join(cmdargs))
        output = subprocess.check_output(cmdargs)
        return json.loads(output)


class Secret:
    def __init__(self, cli: SecretServiceCli, data: SecretData, full_name: str):
        self._cli = cli
        self.data = data

        self.full_name = full_name
        self.id = None
        self.versioned_files = []
        self.ss_ug_admins = []
        self.ss_hostgroups = []

        # Set by sync()
        self.version = None
        self.changed_content = False

    def __repr__(self):
        return "<Secret {} ({})>".format(self.full_name, self.id)

    @classmethod
    def resolve_secrets(cls, cli: SecretServiceCli, secret_data: List[SecretData],
                        prefix: str) -> List["Secret"]:
        uploaded_secret_list = cli.run("secret", "list")
        uploaded_secrets = {uploaded_secret["name"]: uploaded_secret for uploaded_secret
                            in uploaded_secret_list["secrets"]}

        secrets = []
        for data in secret_data:
            full_name = data.name
            if prefix:
                full_name = "{}_{}".format(prefix, full_name)
            full_name = full_name.replace("@", "_")

            secret = cls(cli, data, full_name)
            uploaded_secret = uploaded_secrets.get(full_name)
            if uploaded_secret:
                secret.id = uploaded_secret["id"]

            secrets.append(secret)

        return secrets

    def upsert(self):
        if self.id:
            return

        status = self._cli.run("secret", "add", "-f", self.data.srcpath, "-n", self.full_name)
        if status["status"] != "CREATED":
            raise RuntimeError("Cannot add secret {!s} to secret service!".format(self.data.name))

        self.id = status["id"]
        self.changed_content = True

    def load(self):
        if not self.id:
            raise InternalError("Secret is not upserted!")

        ss_secret = self.run_self("show")
        self.versioned_files = []
        for secret_file in ss_secret["secret"]["files"]:
            for version in secret_file["versions"]:
                versioned_file = version.copy()
                versioned_file["name"] = secret_file["name"]
                self.versioned_files.append(versioned_file)

        self.ss_ug_admins = [ss_ug["name"] for ss_ug in ss_secret.get("usergroups", [])]

        ss_secret_hostgroups = self.run_self("list-hostgroups")
        self.ss_hostgroups = [ss_hostgroup["name"] for ss_hostgroup
                              in ss_secret_hostgroups.get("hostgroups", [])]

    def sync(self):
        if not self.id or not self.versioned_files:
            raise InternalError("Secret is not loaded!")

        versioned_file = self._find_versioned_file(required=False)
        if versioned_file is None:
            self.run_self("update", "-f", self.data.srcpath)

            self.load()
            versioned_file = self._find_versioned_file()
            self.changed_content = True

            logging.info("Updated contents of the secret %s, new version is %s", self.id,
                         versioned_file["version"])

        self.version = versioned_file["version"]

        abc_groups = set()
        for _, base_role in self.data.ss_hostgroups:
            abc_groups.update(self._cli.config.base_role_abc_groups.get(base_role, []))
        for abc_group in abc_groups:
            if abc_group in self.ss_ug_admins:
                continue

            self.run_self("grant", "-g", abc_group, "-r", "admin")
            logging.info("Granted access to secret %s to group %s", self.id, abc_group)

        if any([self.data.USER != versioned_file.get("user"),
                self.data.GROUP != versioned_file.get("group")]):
            self.run_self("set", "--version", self.version, "--filename", self.data.filename,
                          "--user", self.data.USER, "--group", self.data.GROUP)
            logging.info("Changed metadata of the secret %s version %s", self.id, self.version)

        if self.data.expire:
            expire = self._parse_versioned_file_expire(versioned_file)
            if expire is None or self.data.expire.date() != expire.date():
                self.run_self("set", "--version", self.version, "--filename", self.data.filename,
                              "--expire", self.data.expire.strftime(EXPIRE_DATEFMT_OUT))
                logging.info("Changed expiration date of the secret %s version %s to %s", self.id,
                             self.version, self.data.expire.isoformat())

    def assign(self, hostgroup, base_role):
        if hostgroup in self.ss_hostgroups:
            return

        cmdargs = ["hostgroup", "add-secret", hostgroup, "--secret-id", self.id]
        if base_role not in self._cli.config.allowed_base_roles:
            cmd = self._cli.gen(*cmdargs)
            log.warning(("No permissions to assign secret {} to base role {}, "
                         "please ask someone to run:\n\t{}").format(self.full_name, base_role, " ".join(cmd)))
            return

        self._cli.run(*cmdargs)
        logging.info("Assigned secret %s to %s", self.id, hostgroup)

    def run_self(self, op, *cmdargs):
        """Same as self._cli.run, but performs operation `op` for
        this secret."""
        return self._cli.run("secret", op, self.id, *cmdargs)

    def _find_versioned_file(self, required=True):
        sha256 = self.data.get_sha256_b64()
        for versioned_file in self.versioned_files:
            if self.data.expire and self.data.compare_by_expire:
                expire = self._parse_versioned_file_expire(versioned_file)
                if expire and self.data.expire.date() == expire.date():
                    return versioned_file

            if versioned_file["sha256"] == sha256:
                return versioned_file

        if required:
            raise RuntimeError("Cannot find secret with sha256={!r} after update!".format(sha256))

        return None

    def _parse_versioned_file_expire(self, versioned_file):
        expire = versioned_file.get("expireAt")
        if expire is None:
            return None

        return datetime.strptime(expire, EXPIRE_DATEFMT_IN)

    def get_bootstrap_spec(self):
        return dict(
            filter=self.data.bootstrap_filter,
            dst=self.data.dstpath,
            secret_service_id=self.id,
            secret_service_filename=self.data.filename,
            secret_service_version=int(self.version),
        )

    def get_salt_spec(self):
        path = os.path.join(self._cli.config.agent_base_path, self.id,
                            self.data.filename, self.version, self.data.filename)
        spec = dict(id=self.id, src=path)
        if self.data.fingerprints:
            spec["sha1_fingerprint"] = self.data.fingerprints["sha1"]
        if self.data.certificate_common_name:
            spec["certificate_common_name"] = self.data.certificate_common_name

        return spec


def sync_secrets(config: SecretServiceConfig, secrets_data: List[SecretData],
                 cluster: Cluster, secret_group_name: str):
    prefix = cluster.secret_name_prefix
    if not cluster.config.prefix:
        prefix = "{}_{}".format(secret_group_name, prefix)

    cli = SecretServiceCli(config, cluster.config.secret_profile)
    secrets = Secret.resolve_secrets(cli, secrets_data, prefix)
    for secret in secrets:
        secret.upsert()
        secret.load()
        secret.sync()

        for hostgroup, base_role in secret.data.ss_hostgroups:
            secret.assign(hostgroup, base_role)

    return secrets


def dump_secrets_bootstrap(sg_name: str, secrets: List[Secret], stream):
    secret_specs = {secret.full_name: secret.get_bootstrap_spec() for secret in secrets}
    yaml.safe_dump({"secrets": {sg_name: secret_specs}}, stream,
                   default_flow_style=False, sort_keys=False)


def dump_secrets_salt(sg_name: str, secrets: List[Secret], stream):
    secret_specs = defaultdict(dict)
    for secret in secrets:
        secret_spec = secret.get_salt_spec()
        if secret.data.scope:
            secret_specs[secret.data.dstpath][secret.data.scope] = secret_spec
        else:
            secret_specs[secret.data.dstpath] = secret_spec

    yaml.safe_dump({"secrets": {sg_name: dict(secret_specs)}}, stream,
                   default_flow_style=False, sort_keys=False)
