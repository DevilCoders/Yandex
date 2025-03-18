import hashlib
import json
import string
from datetime import datetime, timedelta
from time import time
import random

import flask
from jinja2 import Template
import jwt
from library.python import resource


DEFAULT_TEMPLATE = Template(resource.find("creation/default.json"))


def get_default_config(**kwargs):
    return DEFAULT_TEMPLATE.render(**kwargs)


def generate_token(service_id):
        epoch = datetime.utcfromtimestamp(0)
        payload = {
            'iat': int(time()),
            'exp': int((datetime.now() + timedelta(days=365) - epoch).total_seconds()),
            'iss': "AAB",
            'sub': str(service_id),
        }
        return jwt.encode(payload, flask.current_app.config["PRIVATE_CRYPT_KEY"], algorithm="RS256")


def generate_start_config(service_id, domain):
    secret_key = hashlib.sha256(''.join(random.sample(string.letters, 32))).hexdigest()[:32]
    return json.loads(get_default_config(crypt_secret_key=secret_key,
                                         partner_token=generate_token(service_id),
                                         publisher_domain_regex=domain.replace(".", "\\\\.")))
