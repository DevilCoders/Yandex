# -*- coding: utf-8 -*-
import logging
import logging.handlers
import os
import sys
import uuid
import yaml

from functools import cached_property
from typing import Tuple

from cloud.mdb.infratests.config import InfratestConfig, build_config
from cloud.mdb.infratests.provision.certificator import CertificatorApi, IssueResult
from cloud.mdb.infratests.provision.dataproc import build_and_upload_dataproc_agent
from cloud.mdb.infratests.provision.dns import create_records, delete_records, DnsRecord, list_records
from cloud.mdb.infratests.provision.iam import create_iam_jwt
from cloud.mdb.infratests.provision.kubernetes import list_external_load_balancers
from cloud.mdb.infratests.provision.values import fill_values
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.lockbox import LockboxClient, LockboxClientConfig, PayloadEntryChange, Secret
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


def provision():
    config = build_config()
    logger = build_logger(config)
    stand = Stand(config, logger)
    action = os.getenv('ACTION', 'list_records')
    if action == 'list_records':
        record_sets = list_records(config, logger)
        logger.info("Found following dns records:")
        for rs in record_sets:
            logger.info(f"  {rs.name} => {rs.data}")
    elif action == 'fill_values':
        stand.fill_values()
    elif action == 'create_dns_records':
        stand.create_dns_records()
    elif action == 'generate_yc_config':
        stand.generate_yc_config()
    elif action == 'update_dataproc_agent':
        stand.update_dataproc_agent()
    elif action == 'put_dataproc_ssh_keys':
        stand.put_dataproc_ssh_keys()
    elif action == 'prepare':
        stand.prepare()
    elif action == 'delete_stand':
        stand.delete_stand()
    else:
        print(f"Unknown action \"{action}\"")


class Stand:
    def __init__(self, config: InfratestConfig, logger: logging.Logger):
        self.config = config
        self.logger = logger
        self.cert_api = CertificatorApi(self.config, self.logger)
        # There's some limitation within certificator and if we try to create certificate for domain like
        # "*.private-api.vlbel-long-stand.mdb-infratest.cloud-preprod.yandex.net" we get error
        #   Error Parsing Request  The request subject name is invalid or too long.
        # So we are going to use shorter base domain for subject name and put desired full name to alt names.
        self.cert_base_domain = f"{config.stand_name}.infratest.db.yandex.net"

        self.public_cert_secret_name = f"public-api-cert-for-{self.config.stand_name}"
        self.private_cert_secret_name = f"private-api-cert-for-{self.config.stand_name}"

    @cached_property
    def lockbox_client(self) -> LockboxClient:
        client_config = LockboxClientConfig(
            transport=grpcutil.Config(
                url=self.config.lockbox.url,
                cert_file=self.config.lockbox.cert_file,
                server_name=self.config.lockbox.server_name,
                insecure=self.config.lockbox.insecure,
            ),
            timeout=self.config.lockbox.timeout,
        )
        iam_jwt = create_iam_jwt(self.config, self.logger)
        return LockboxClient(
            config=client_config,
            logger=MdbLoggerAdapter(self.logger, extra={}),
            token_getter=iam_jwt.get_token,
            error_handlers={},
        )

    def fill_values(self):
        public_api_cert, private_api_cert = self._issue_certificates()
        public_secret = self._create_tls_secret(self.public_cert_secret_name, public_api_cert)
        private_secret = self._create_tls_secret(self.private_cert_secret_name, private_api_cert)
        fill_values(self.config, public_api_cert_secret=public_secret, private_api_cert_secret=private_secret)

    def create_dns_records(self):
        balancers = list_external_load_balancers(self.config)
        ip_by_service_name = {balancer.name: balancer.ip for balancer in balancers}

        records = [
            DnsRecord('api', ip_by_service_name['public-api']),
            DnsRecord('*.api', ip_by_service_name['public-api']),
            DnsRecord('mdb-internal-api.private-api', ip_by_service_name['go-api']),
            DnsRecord('mdb.private-api', ip_by_service_name['py-api']),
        ]
        self.logger.info("Following dns records will be created if absent:")
        for record in records:
            self.logger.info(f"  {record.name_prefix} => {record.ip}")
        create_records(self.config, self.logger, records)

    def _issue_certificates(self) -> Tuple[IssueResult, IssueResult]:
        private_api_cert = self.cert_api.issue(
            f"private-api.{self.cert_base_domain}", [f"*.private-api.{self.config.base_domain}"]
        )

        public_api_cert = self.cert_api.issue(
            f"api.{self.cert_base_domain}", [f"api.{self.config.base_domain}", f"*.api.{self.config.base_domain}"]
        )

        return public_api_cert, private_api_cert

    def _create_tls_secret(self, secret_name: str, cert: IssueResult) -> Secret:
        entries = [
            PayloadEntryChange(key='key', text_value=cert.key),
            PayloadEntryChange(key='crt', text_value=cert.cert),
        ]
        secret = self.lockbox_client.get_secret_by_name(self.config.folder_id, secret_name)
        if secret:
            self.lockbox_client.create_version(
                secret_id=secret.id,
                payload_entries=entries,
                idempotency_key=str(uuid.uuid4()),
            )
            # reload secret to refresh current_version
            secret = self.lockbox_client.get_secret_by_name(self.config.folder_id, secret_name)
        else:
            secret, _ = self.lockbox_client.create_secret(
                folder_id=self.config.folder_id,
                name=secret_name,
                version_payload_entries=entries,
                idempotency_key=str(uuid.uuid4()),
            )
        return secret

    def delete_stand(self):
        delete_records(self.config, self.logger)
        self._revoke_certificates()
        self._delete_lockbox_secrets()

    def _revoke_certificates(self):
        self.cert_api.revoke(f"api.{self.cert_base_domain}")
        self.cert_api.revoke(f"private-api.{self.cert_base_domain}")

    def _delete_lockbox_secrets(self):
        for secret_name in [self.public_cert_secret_name, self.private_cert_secret_name]:
            secret = self.lockbox_client.get_secret_by_name(self.config.folder_id, secret_name)
            if secret:
                self.logger.info(f"Deleting secret id={secret.id} name={secret.name}")
                self.lockbox_client.delete_secret(secret.id, idempotency_key=str(uuid.uuid4()))

    def generate_yc_config(self):
        sa = self.config.user_service_account
        config = {
            'current': 'default',
            'profiles': {
                'default': {
                    'endpoint': f"api.{self.config.base_domain}:443",
                    'folder-id': self.config.folder_id,
                    'service-account-key': {
                        'id': sa.key_id,
                        'service_account_id': sa.id,
                        'key_algorithm': 'RSA_2048',
                        'private_key': sa.private_key,
                    },
                },
            },
        }
        with open(self.config.yc_config_path, 'w') as file:
            yaml.dump(config, file)
        self.logger.info(f'YC CLI config is saved to {self.config.yc_config_path}')

    def update_dataproc_agent(self):
        build_and_upload_dataproc_agent(self.config, self.logger)

    def put_dataproc_ssh_keys(self):
        with open(self.config.dataproc.ssh_key.private_path, 'w') as outfile:
            outfile.write(self.config.dataproc.ssh_key.private)
            self.logger.info(f'Private SSH key for Data Proc is saved to {self.config.dataproc.ssh_key.private_path}')
        with open(self.config.dataproc.ssh_key.public_path, 'w') as outfile:
            outfile.write(self.config.dataproc.ssh_key.public)
            self.logger.info(f'Public SSH key for Data Proc is saved to {self.config.dataproc.ssh_key.public_path}')

    def prepare(self):
        self.generate_yc_config()
        self.put_dataproc_ssh_keys()
        self.update_dataproc_agent()


def build_logger(config: InfratestConfig):
    logger = logging.getLogger(__name__)
    logger.setLevel(config.logging.level)

    handler = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(asctime)s [%(levelname)s]\t%(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger
