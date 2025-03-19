# -*- coding: utf-8 -*-
import os
import subprocess
from dataclasses import dataclass
from getpass import getuser

from cloud.mdb.internal.python.vault import YandexVault


@dataclass
class CertificatorConfig:
    url: str
    token: str
    ca_name: str
    cert_type: str


@dataclass
class SshKey:
    private: str
    private_path: str
    public: str
    public_path: str


@dataclass
class DataprocConfig:
    agent_builds_bucket_name: str
    agent_repo: str
    agent_sa_id: str
    agent_sa_2_id: str
    agent_version: str
    default_subnet_id: str
    log_group_id: str
    output_bucket: str
    output_bucket_2: str
    sa_without_role: str
    ssh_key: SshKey
    zone: str


@dataclass
class DnsConfig:
    url: str
    zone_id: str
    cert_file: str = None
    server_name: str = None
    insecure: bool = False
    timeout: int = 10


@dataclass
class IaaSConfig:
    compute_url: str


@dataclass
class IamJwtConfig:
    url: str
    audience: str = 'https://iam.api.cloud.yandex.net/iam/v1/tokens'
    cert_file: str = '/opt/yandex/allCAs.pem'
    expire_thresh: int = 180
    insecure: bool = False
    request_expire: int = 3600
    server_name: str = ''


@dataclass
class LockboxConfig:
    url: str
    cert_file: str = None
    server_name: str = None
    insecure: bool = False
    timeout: int = 10


@dataclass
class LoggingConfig:
    level: str = 'INFO'


@dataclass
class ServiceAccountConfig:
    id: str
    key_id: str
    private_key: str
    s3_access_key: str = ''
    s3_secret_key: str = ''


@dataclass
class S3Config:
    endpoint_url: str
    region_name: str


@dataclass
class InfratestConfig:
    arcadia_root: str
    base_domain: str
    ca_path: str
    certificator: CertificatorConfig
    cloud_id: str
    cloud_resources_owner: str
    dataproc: DataprocConfig
    dns: DnsConfig
    folder_id: str
    iaas: IaaSConfig
    iam_jwt: IamJwtConfig
    helmfile_dir_path: str
    lockbox: LockboxConfig
    logging: LoggingConfig
    provisioner_service_account: ServiceAccountConfig
    py_api_url: str
    stand_name: str
    s3: S3Config
    # Credentials of SA mdb-user that emulates user SA (and calls our APIs within infratests)
    user_service_account: ServiceAccountConfig
    yc_bin_path: str
    yc_config_path: str


def build_config() -> InfratestConfig:
    stand_name = os.environ['STAND']
    helmfile_dir_path = os.environ['HELMFILE_DIR_PATH']
    yav = YandexVault()
    base_domain = f'{stand_name}.mdb-infratest.cloud-preprod.yandex.net'
    arcadia_root = subprocess.check_output(['arc', 'root']).decode("utf-8").strip()
    infratests_path = f'{arcadia_root}/cloud/mdb/infratests'
    ca_path = f'{arcadia_root}/cloud/mdb/salt/salt/components/common/conf/allCAs.pem'
    return InfratestConfig(
        arcadia_root=arcadia_root,
        base_domain=base_domain,
        ca_path=ca_path,
        certificator=CertificatorConfig(
            url='https://crt-api.yandex-team.ru',
            token=yav.get('sec-01dw7fewxkg794rf0sm1g8brqh')['token'],
            ca_name='YcInternalCA',
            cert_type='yc-server',
        ),
        cloud_id='aoe9shbqc2v314v7fp3d',  # name: mdb
        cloud_resources_owner=getuser(),
        dataproc=DataprocConfig(
            agent_builds_bucket_name='dataproc-infratest-agent-builds',
            agent_repo='https://storage.cloud-preprod.yandex.net/dataproc-infratest-agent-builds',
            agent_sa_id='bfbbus0ujfjf8hdsol6t',
            agent_sa_2_id='bfbf20oblad9vj6bfn0p',
            agent_version=f'0.{stand_name}',
            default_subnet_id='buc6q84e48hmmddr2fk4',
            log_group_id='af335npal00idhlbid1b',  # name: dataproc
            output_bucket='dataproc-infratest',
            output_bucket_2='dataproc-infratest-2',
            sa_without_role='bfbn7nv215mq6rvc502j',
            ssh_key=SshKey(
                private=yav.get('sec-01dtve9nqnp6am8m2063ras3tt')['key'],
                private_path=f'{infratests_path}/staging/dataproc_ssh_key',
                public='ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAII7JOBFU5LGCd/ET220neX7MiWIXHnZI9ZfFjjgnPMmh',
                public_path=f'{infratests_path}/staging/dataproc_ssh_key.pub',
            ),
            zone='ru-central1-a',
        ),
        dns=DnsConfig(
            url='dns.private-api.ycp.cloud-preprod.yandex.net',
            zone_id='aet0bb9bnt0kda1q06oi',
        ),
        folder_id='aoeh90d3b0i4dlj6dl1s',  # name: infra-tests
        iaas=IaaSConfig(
            compute_url='compute-api.cloud-preprod.yandex.net:9051',
        ),
        iam_jwt=IamJwtConfig(
            url='ts.private-api.cloud-preprod.yandex.net:4282',
            cert_file=ca_path,
        ),
        helmfile_dir_path=helmfile_dir_path,
        lockbox=LockboxConfig(
            url='lockbox.cloud-preprod.yandex.net:8443',
        ),
        logging=LoggingConfig(
            level='INFO',
        ),
        provisioner_service_account=ServiceAccountConfig(
            id='bfbc5f43q70e73enn9q8',
            key_id='bfb2p6gq6linesli38kf',
            private_key=yav.get('sec-01g0ydd0wxevjzh0k60ee8rcse')['private_key'],
            s3_access_key=yav.get('sec-01g0ydd0wxevjzh0k60ee8rcse')['s3_access_key'],
            s3_secret_key=yav.get('sec-01g0ydd0wxevjzh0k60ee8rcse')['s3_secret_key'],
        ),
        py_api_url=f'mdb.private-api.{base_domain}',
        stand_name=stand_name,
        s3=S3Config(
            endpoint_url='storage.cloud-preprod.yandex.net',
            region_name='ru-central1',
        ),
        user_service_account=ServiceAccountConfig(
            id='bfbp6i88k3ou09vki6fs',
            key_id='bfbh275j7u086i8ebb1l',
            private_key=yav.get('sec-01g38vm86brjc940tvcnqn0gd3')['private_key'],
            s3_access_key=yav.get('sec-01g38vm86brjc940tvcnqn0gd3')['s3_access_key'],
            s3_secret_key=yav.get('sec-01g38vm86brjc940tvcnqn0gd3')['s3_secret_key'],
        ),
        yc_bin_path=f'{infratests_path}/staging/bin/yc',
        yc_config_path=f'{infratests_path}/staging/yc_config.yaml',
    )
