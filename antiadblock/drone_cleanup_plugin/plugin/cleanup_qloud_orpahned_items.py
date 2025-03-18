"""
@author mavlyutov
@requires requests, urlbuilder

Script to cleanup qloud environments
"""
import os

import requests

from operator import itemgetter

from requests.auth import AuthBase
from urlbuilder import URLBuilder


class OAuth(AuthBase):

    def __init__(self, token):
        self.token = token

    def __call__(self, r):
        r.headers['Authorization'] = ' '.join(['OAuth', self.token])
        return r


URL = URLBuilder('https://platform.yandex-team.ru/api/v1/')
AUTH = OAuth(os.getenv('QLOUD_PASSWORD'))
APPLICATION = os.getenv('PLUGIN_APPLICATION')
MAX_ENVIRONMENTS = int(os.getenv('PLUGIN_MAX_ENVIRONMENTS', 10))


if __name__ == "__main__":
    if not APPLICATION:
        print "application name should be specified"
        exit(1)

    if not AUTH.token:
        print "QLOUD_PASSWORD token is not specified"
        exit(1)

    response = requests.get(URL.application[APPLICATION], auth=AUTH)
    if response.status_code != 200:
        print "QLOUD request was failed with {} code:".format(response.status_code)
        print response.content
        exit(1)
    raw_environments = requests.get(URL.application[APPLICATION], auth=AUTH).json()
    environments = [env['objectId'] for env in raw_environments['environments']]

    print "Found {count} demos. They are: {demos}".format(count=len(environments), demos=", ".join(environments))

    if len(environments) <= MAX_ENVIRONMENTS:
        print "Max demos is {}. Do nothing".format(MAX_ENVIRONMENTS)
        exit(0)

    environments = [(env, requests.get(URL.environment.stable[env], auth=AUTH).json()['creationDate']) for env in environments]
    # do not remove 'master' or 'develop' stands
    environments = filter(lambda env: not any(s in env[0] for s in ['master', 'develop', 'production']), environments)

    for env_to_delete in sorted(environments, key=itemgetter(1))[:-MAX_ENVIRONMENTS]:
        print 'deleting', env_to_delete[0]
        requests.delete(URL.environment.stable[env_to_delete[0]], auth=AUTH)
