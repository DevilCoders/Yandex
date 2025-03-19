import os
import re
import time
import jwt
import json
import logging.config
import nirvana_dl
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)



def getToken(private_key):
    service_account_id = "aje2odfnho9l70ap2dfe"
    key_id = "ajelt3n5dcras06du3p7" # ID ресурса Key, который принадлежит сервисному аккаунту.

    now = int(time.time())
    payload = {
            'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            'iss': service_account_id,
            'iat': now,
            'exp': now + 360}

    # Формирование JWT.
    encoded_token = jwt.encode(
        payload,
        private_key,
        algorithm='PS256',
        headers={'kid': key_id})
    return encoded_token


def main() -> None:

    private_key = nirvana_dl.get_options()['user_requested_secret']
    private_key = re.sub("\*", "\n", private_key)
    encoded_token = getToken(private_key)
    os.system('''curl -X POST -H 'Content-Type: application/json' -d '{"jwt": "''' + encoded_token + '''"}' https://iam.api.cloud.yandex.net/iam/v1/tokens -o iam_token.json''')
    data = json.load(open('iam_token.json'))
    logger.debug(data['iamToken'])
    os.system(f'''curl -H "X-YaCloud-SubjectToken: {data['iamToken']}" -H 'Content-Type: application/json' -X GET https://billing.private-api.cloud.yandex.net:16465/billing/v1/private/billingAccounts/dn24vr4lfuuq4t6fdi6n/transactions -o {nirvana_dl.json_output_file()}''')


if __name__ == '__main__':
    main()
