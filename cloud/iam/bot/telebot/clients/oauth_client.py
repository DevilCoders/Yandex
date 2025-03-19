from cloud.iam.bot.telebot.clients import BaseClient


class OAuth():
    def __init__(self, token):
        self._token = token

    def __call__(self, request):
        request.headers['Authorization'] = 'OAuth ' + self._token
        return request


class OAuthClient(BaseClient):
    def __init__(self, config, token):
        super().__init__(config)
        self._token = token

    def _create_session(self):
        result = super()._create_session()
        result.auth = OAuth(self._token)
        return result
