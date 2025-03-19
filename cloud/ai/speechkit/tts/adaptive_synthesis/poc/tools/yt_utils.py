import os


class Config:
    ARCADIA_PATH = os.path.expanduser('~/arcadia')

    YT_TOKEN_NAME = 'd-kruchinin-yt-token'
    NIRVANA_TOKEN_NAME = 'd-kruchinin-nirvana-token'
    YT_PROXY = 'hahn'

    @staticmethod
    def yt_token():
        with open(os.path.expanduser('~/.yt/token'), 'r') as token_file:
            token = token_file.read()[:-1]
        return token


def set_yt_config(yt_config):
    yt_config['proxy']['url'] = Config.YT_PROXY
    yt_config['token'] = Config.yt_token()
