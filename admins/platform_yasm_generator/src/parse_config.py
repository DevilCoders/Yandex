from pathlib import Path
import sys
import yaml
import logging as log


def parse_config(conf: Path):
    log.info(f"Parsing config {conf.resolve()}")
    config = None
    try:
        with conf.open() as fobj:
            config = yaml.safe_load(fobj)
    except Exception as ex:
        log.fatal(str(ex))
        sys.exit()
    return config
