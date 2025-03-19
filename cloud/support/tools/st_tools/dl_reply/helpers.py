from startrek_client import exceptions as st_exceptions
from startrek_client import Startrek

def get_token():
    with open('.st_token', 'r') as f:
        token = f.read()
    return token.strip()

