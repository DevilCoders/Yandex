from typing import Dict
import json
import logging


def load_config(ctx, file_path: str) -> Dict:
    logging.debug("Loading config: %s", file_path)
    with open(file_path) as fh:
        return json.load(fh)
