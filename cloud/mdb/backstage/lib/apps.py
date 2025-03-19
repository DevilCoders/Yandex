from django.conf import settings


class App:
    def __init__(self, name):
        self.name = name
        self.is_enabled = name in settings.ENABLED_BACKSTAGE_APPS


CMS = App('cms')
META = App('meta')
DEPLOY = App('deploy')
KATAN = App('katan')
DBM = App('dbm')
MLOCK = App('mlock')


ALL = [
    CMS,
    META,
    DEPLOY,
    KATAN,
    DBM,
    MLOCK,
]
