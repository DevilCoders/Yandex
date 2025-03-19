import logging

from urllib.parse import urlparse, urlunparse

from cloud.iam.bot.telebot.clients import BaseClient


class Paste(BaseClient):
    def post(self, syntax, text):
        response = self.session.post(self._config['api'],
                                     headers={'Content-Type': 'application/x-www-form-urlencoded'},
                                     data={'syntax': syntax, 'text': text},
                                     allow_redirects=False,
                                     timeout=self._config.get('timeout'))

        logging.info(response)
        logging.info(response.text)

        if response.status_code != 302:
            raise RuntimeError('Error uploading to paste.yandex-team.ru')

        logging.info(response.text)

        url = response.headers['Location']
        url_tuple = urlparse(url)
        if url_tuple.netloc:
            return url

        server_tuple = urlparse(self._config['api'])
        url_tuple = (server_tuple[0], server_tuple[1],
                     url_tuple[2], url_tuple[3], url_tuple[4], url_tuple[5])

        return urlunparse(url_tuple)
