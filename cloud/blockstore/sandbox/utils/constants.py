import sandbox.common.types.client as ctc


DEFAULT_ARTIFACTS_DIRECTORY = 'artifacts'
DEFAULT_TEAMCITY_ARTIFACTS_TYPE = 'TEAMCITY_ARTIFACTS'
DEFAULT_TEAMCITY_SERVICE_MESSAGES_LOG_TYPE = 'TEAMCITY_SERVICE_MESSAGES_LOG'
DEFAULT_TEAMCITY_ARTIFACTS_TTL = 7
DEFAULT_TEAMCITY_SERVICE_MESSAGES_LOG_TTL = 7

DEFAULT_REQUIREMENTS_CORES = 1
DEFAULT_REQUIREMENTS_DISK_SPACE = 1 * (2 ** 10)  # MiB
DEFAULT_REQUIREMENTS_RAM = 1 * (2 ** 10)  # MiB
DEFAULT_REQUIREMENTS_CLIENT_TAG = (
    (ctc.Tag.GENERIC | ctc.Tag.Group.OSX | ctc.Tag.PORTOD) &
    (
        ~ctc.Tag.Group.LINUX | ctc.Tag.LINUX_PRECISE | ctc.Tag.LINUX_TRUSTY |
        ctc.Tag.LINUX_XENIAL | ctc.Tag.LINUX_BIONIC | ctc.Tag.LINUX_FOCAL
    )
)

DEFAULT_VCS_MOUNT_POINT = 'arcadia'
DEFAULT_VCS_STORE_POINT = 'store'

DEFAULT_FIND_TARGET = 'cloud/blockstore/teamcity/bin'
DEFAULT_FIND_TASKS_BUNDLE = 'NBS_TEAMCITY_RUNNER'

DEFAULT_CONFIG_COMMON_FILE = 'common.yaml'
DEFAULT_CONFIG_SCHEMA_RUNNER = {
    "type": "object",
    "properties": {
        "task_configs": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "name": {"type": "string"},
                    "config_path": {"type": "string"},
                    "artifacts": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "source_path": {"type": "string"},
                                "source_type": {"type": "string"},
                                "destination_path": {"type": "string"},
                                "destination_type": {"type": "string"},
                            },
                        },
                    },
                    "depends_on": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": {"type": "string"},
                                "artifacts": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "type": {"type": "string"},
                                            "parameter_name": {"type": "string"},
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
    },
    "required": ["task_configs"]
}
DEFAULT_CONFIG_SCHEMA_TASK = {
    "type": "object",
    "properties": {
        "task_type": {"type": "string"},
        "requirements": {
            "type": "object",
            "properties": {
                "cores": {"type": "number"},
                "ram": {"type": "number"},
                "disk_space": {"type": "number"},
                "container_resource": {"type": "number"},
            },
        },
        "parameters": {"type": "object"},
    },
    "required": ["requirements", "parameters"]
}
