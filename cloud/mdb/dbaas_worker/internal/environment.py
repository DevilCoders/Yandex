from enum import Enum


class EnvironmentName(Enum):
    PORTO = 'porto'
    COMPUTE = 'compute'


def get_env_name_from_config(config) -> EnvironmentName:
    return EnvironmentName(config.environment_name)
