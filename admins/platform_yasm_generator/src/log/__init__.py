from admins.yaconfig import load_config
from pathlib import Path

try:
    from library.python import resource
except:
    pass


def setup_logging():
    try:
        load_config(Path("logging.yaml"))
    except:
        res = resource.find("/logging")
        load_config(res)
