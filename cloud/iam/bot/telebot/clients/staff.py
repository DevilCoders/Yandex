import json
import logging
import re
import requests

from cloud.iam.bot.telebot.clients import TvmClient


class Staff(TvmClient):
    application_id = '2001974'

    def get_account(self, tg_username):
        logging.debug('Telegram username "{}"'.format(tg_username))

        # Reject invalid usernames.
        if not re.match(r'[A-Za-z0-9_]{5,32}', tg_username):
            return None

        request_fields = 'login,official.is_dismissed,groups.group.type,groups.group.url'

        response = self.session.get(self._config['api'] +
                                    f'persons?telegram_accounts.value={tg_username}&_one=1&_fields={request_fields}',
                                    timeout=self._config.get('timeout'))

        if response.status_code == requests.codes.not_found:
            return None

        if not response.ok:
            response.raise_for_status()

        result = json.loads(response.text)

        if result.get('official', {}).get('is_dismissed', True):
            return None

        return result
