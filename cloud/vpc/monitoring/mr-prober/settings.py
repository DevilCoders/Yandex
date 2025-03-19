import logging
import os
import pathlib
import socket

from environs import Env

from common import urllib3_use_only_ipv6_if_available

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

env = Env(expand_vars=True)
env.read_env()  # read .env file, if it exists

DATABASE_URL = env("DATABASE_URL", "sqlite:///" + os.path.join(BASE_DIR, "database.sqlite"))

is_sqlite = DATABASE_URL.startswith("sqlite://")
is_postgresql = DATABASE_URL.startswith("postgresql://")
DATABASE_CONNECT_ARGS = env.json(
    "DATABASE_CONNECT_ARGS", '{"check_same_thread": false}' if is_sqlite else '{"options": "-c timezone=utc"}'
)

DEBUG = env.bool("DEBUG", True)
BOTO3_DEBUG = env.bool("BOTO3_DEBUG", False)

URLLIB3_USE_ONLY_IPV6_IF_AVAILABLE = env.bool("URLLIB3_USE_ONLY_IPV6_IF_AVAILABLE", False)

LOG_LEVEL = env.log_level("LOG_LEVEL", logging.DEBUG if DEBUG else logging.INFO)
LOG_FORMAT = "%(asctime)s.%(msecs)03d [%(threadName)s:%(name)s] %(levelprefix)s %(message)s"
PROBER_LOG_FORMAT = "%(asctime)s.%(msecs)03d %(message)s"
ENABLE_COLORS_IN_LOGS = False
LOG_DATE_FORMAT = "%H:%M:%S" if DEBUG else "%Y-%m-%d %H:%M:%S"

LOGGING_CONFIG = {
    "version": 1,
    "disable_existing_loggers": True,
    "formatters": {
        # colorful formatter
        "standard": {
            "()": "uvicorn.logging.DefaultFormatter",
            "format": LOG_FORMAT,
            "datefmt": LOG_DATE_FORMAT,
            "use_colors": ENABLE_COLORS_IN_LOGS,
        },
        "uvicorn.access": {
            "()": "uvicorn.logging.AccessFormatter",
            "format": '%(asctime)s.%(msecs)03d [access] %(levelprefix)s %(client_addr)s - "%(request_line)s" %(status_code)s',
            "datefmt": LOG_DATE_FORMAT,
            "use_colors": ENABLE_COLORS_IN_LOGS,
        },
    },
    "handlers": {
        "default": {
            "level": LOG_LEVEL,
            "formatter": "standard",
            "class": "logging.StreamHandler",
        },
        "uvicorn.access": {
            "level": LOG_LEVEL,
            "formatter": "uvicorn.access",
            "class": "logging.StreamHandler",
        }
    },
    "loggers": {
        # root logger
        "": {"handlers": ["default"], "level": LOG_LEVEL, "propagate": False},

        "urllib3": {"level": LOG_LEVEL},
        "fastapi": {"propagate": True},
        "uvicorn": {"propagate": True},
        "uvicorn.access": {"handlers": ["uvicorn.access"], "propagate": False},
        "uvicorn.asgi": {"propagate": True},
        "uvicorn.error": {"propagate": True},

        "gunicorn.access": {"handlers": ["uvicorn.access"], "propagate": False},
        "gunicorn.error": {"propagate": True},

        # Boto3 writes too many logs
        "botocore": {
            "level": logging.DEBUG if BOTO3_DEBUG else logging.WARNING,
            "handlers": ["default"],
            "propagate": False
        },
        "boto3": {
            "level": logging.DEBUG if BOTO3_DEBUG else logging.WARNING,
            "handlers": ["default"],
            "propagate": False
        },
        "s3transfer": {
            "level": logging.DEBUG if BOTO3_DEBUG else logging.WARNING,
            "handlers": ["default"],
            "propagate": False
        },
        "multipart": {
            "level": logging.DEBUG if BOTO3_DEBUG else logging.INFO,
            "propagate": False,
        }
    }
}

JAEGER_HOST = env("JAEGER_HOST", "")  # Sending traces to jaeger is disabled by default
JAEGER_PORT = env.int("JAEGER_PORT", 6831, validate=lambda port: 0 <= port < 65536)

API_KEY = env("API_KEY", "12345678")

MR_PROBER_LOGS_S3_BUCKET = env.str("MR_PROBER_LOGS_S3_BUCKET", "mr-prober-logs")

GRPC_ROOT_CERTIFICATES_PATH = env.path("GRPC_ROOT_CERTIFICATES_PATH", "/etc/ssl/certs/YandexInternalRootCA.pem")
API_ROOT_CERTIFICATES_PATH = env.path("API_ROOT_CERTIFICATES_PATH", "/etc/ssl/certs/YandexInternalRootCA.pem")

SOLOMON_AGENT_ENDPOINT = "http://[::1]:10050/write"
SEND_METRICS_TO_SOLOMON = env.bool("SEND_METRICS_TO_SOLOMON", not DEBUG)

# Creator-specific settings

# Should be created in https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage
TERRAFORM_STATES_BUCKET_NAME = env.str("TERRAFORM_STATES_BUCKET_NAME", "mr-prober-cluster-states")
CONDUCTOR_TOKEN = env("CONDUCTOR_TOKEN", "")
MR_PROBER_SA_AUTHORIZED_KEY = env.json("MR_PROBER_SA_AUTHORIZED_KEY", "{}")
CREATOR_TELEMETRY_SENDING_INTERVAL = env.float("CREATOR_TELEMETRY_SENDING_INTERVAL", 30)

# API-specific settings

HOST = env("HOST", "127.0.0.1")
PORT = env.int("PORT", 8080, validate=lambda port: 0 <= port < 65536)
# Additional port for healthchecks. Both ports will be bounded to the same application
ADDITIONAL_PORT = env.int("ADDITIONAL_PORT", None, validate=lambda port: 0 <= port < 65536 and port != PORT)

API_AUTO_RELOAD = env.bool("API_AUTO_RELOAD", DEBUG)
API_WORKERS_PER_CORE = env.float("API_WORKERS_PER_CORE", 1)

# Agent-specific settings

CLUSTER_ID = env.int("CLUSTER_ID", None)
HOSTNAME = env.str("HOSTNAME", socket.getfqdn())
UPDATE_AGENT_CONFIG_FROM_S3_INTERVAL_SECONDS = 60
PROBER_FILES_PATH = env("PROBER_FILES_PATH", os.path.join(BASE_DIR, "/tmp/mr_prober/agent/downloaded_prober_files"))
pathlib.Path(PROBER_FILES_PATH).mkdir(parents=True, exist_ok=True)

DEFAULT_PROBER_TIMEOUT_SECONDS = env.int("DEFAULT_PROBER_TIMEOUT_SECONDS", 10)
IP_TOOL_PATH = "ip"
# iptables (without -legacy) is a frontend to netfilter in modern versions of Ubuntu. This version
# doesn't support cgroup filtering now, so we have to use iptables-legacy with old backend.
#
# $ iptables -t mangle -A OUTPUT -m cgroup --cgroup 0xd00d0002 -j MARK --set-mark 2
# iptables v1.8.7 (nf_tables): Couldn't load match `cgroup':No such file or directory
# Try `iptables -h' or 'iptables --help' for more information.
IPTABLES_TOOL_PATH = "iptables-legacy"
IP6TABLES_TOOL_PATH = "ip6tables-legacy"

AGENT_GET_CLUSTER_CONFIG_RETRY_ATTEMPTS = env.int("AGENT_GET_CONFIG_RETRY_ATTEMPTS", 24)
AGENT_GET_CLUSTER_CONFIG_RETRY_DELAY = env.float("AGENT_GET_CONFIG_RETRY_DELAY", 300)

# NOTE: There is a reference to parameter AGENT_TELEMETRY_SENDING_INTERVAL in custom alerts for some clusters:
# https://solomon.yandex-team.ru/admin/projects/cloud_mr_prober/alerts/
AGENT_TELEMETRY_SENDING_INTERVAL = env.float("AGENT_TELEMETRY_SENDING_INTERVAL", 30)
AGENT_CONFIGURATIONS_S3_BUCKET = env.str("AGENT_CONFIGURATIONS_S3_BUCKET", "mr-prober-agent-configurations")
AGENT_PROBER_MAX_OUTPUT_SIZE = env.int("AGENT_PROBER_MAX_OUTPUT_SIZE", 1024 * 1024 * 10)  # 10 MiB
AGENT_ADDITIONAL_METRIC_LABELS = env.json("AGENT_ADDITIONAL_METRIC_LABELS", "{}")
assert isinstance(AGENT_ADDITIONAL_METRIC_LABELS, dict), "AGENT_ADDITIONAL_METRIC_LABELS must contain a valid dict"

AGENT_DOCKER_IMAGE = env.str("AGENT_DOCKER_IMAGE", None, allow_none=True)

AGENT_TTY_DEVICE = env.str("AGENT_TTY_DEVICE", None, allow_none=True)

# Common logs dir

MR_PROBER_LOGS_PATH = env("MR_PROBER_LOGS_PATH", os.path.join(BASE_DIR, "logs"))
pathlib.Path(MR_PROBER_LOGS_PATH).mkdir(parents=True, exist_ok=True)

# Logs of prober runs

MR_PROBER_PROBER_LOGS_PATH = os.path.join(MR_PROBER_LOGS_PATH, "probers")

# API- and agent-specific settings

S3_ENDPOINT = env.str("S3_ENDPOINT", "https://storage.yandexcloud.net")
S3_ACCESS_KEY_ID = env.str("S3_ACCESS_KEY_ID", "")
S3_SECRET_ACCESS_KEY = env.str("S3_SECRET_ACCESS_KEY", "")
S3_PREFIX = env.str("S3_PREFIX", f"development/{socket.getfqdn()}/")
S3_CONNECT_TIMEOUT = env.float("S3_CONNECT_TIMEOUT", 10)  # seconds
S3_RETRY_ATTEMPTS = env.int("S3_RETRY_ATTEMPTS", 3)

# CLI settings
RICH_FORCE_TERMINAL = env.bool("RICH_FORCE_TERMINAL", None, allow_none=True)
RICH_WIDTH = env.int("RICH_WIDTH", None, allow_none=True)

# CLOUD-80463. Override the method responsible for selecting the address family in urllib3 to disable ipv4 for botocore
if URLLIB3_USE_ONLY_IPV6_IF_AVAILABLE:
    import urllib3.util.connection

    urllib3.util.connection.allowed_gai_family = urllib3_use_only_ipv6_if_available
