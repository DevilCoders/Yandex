import logging
import os
from typing import Tuple, List, Literal


def set_logger(env):
    level = logging.DEBUG
    if env == "prod":
        level = logging.INFO
    logging.basicConfig(
        level=level, format="[%(levelname)s][%(asctime)s][%(funcName)s] %(message)s", datefmt='%D %H:%M:%S'
    )


def get_messenger_and_environment(
    argv: List[str],
) -> Tuple[Literal["telegram", "yandex", "slack"], Literal["prod", "preprod", "debug"]]:
    valid_messengers = ["telegram", "yandex", "slack"]
    valid_envs = ["prod", "preprod", "debug"]

    env = os.getenv("ENVIRONMENT")
    messenger = os.getenv("MESSENGER")

    if env is None or messenger is None:
        if len(argv) < 3:
            raise ValueError(
                f"Set messenger ({valid_messengers}) and environment ({valid_envs}) "
                f"in $MESSENGER and $ENVIRONMENT, "
                f"or run ./bot.py <messenger> <environment>, i.e. ./bot.py telegram debug"
            )
        messenger, env = argv[1:]

    if messenger not in valid_messengers:
        raise ValueError(f"Messenger {messenger} is not valid. Valid envs: {valid_messengers}")
    if env not in valid_envs:
        raise ValueError(f"Env {env} is not valid. Valid envs: {valid_envs}")

    return messenger, env
