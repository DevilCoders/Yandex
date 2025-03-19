TYPE_COMPUTE = 'compute'
TYPE_CLOUDIL = 'cloudil'
TYPE_PORTO = 'porto'
TYPE_DC = 'dc'


TYPE_TO_IMAGE = {
    TYPE_PORTO: 'yandex.png',
    TYPE_COMPUTE: 'yandex_cloud.png',
    TYPE_DC: 'doublecloud.png',
    TYPE_CLOUDIL: 'cloudil.png',
}

NAMES = [
    'porto-test',
    'porto-prod',
    'compute-preprod',
    'compute-prod',
    'dc-preprod',
    'dc-prod',
    'cloudil-prod',
]


class Installation:
    def __init__(
        self,
        type,
        name,
        url,
        classic_url=None,
        managed_domain=None,
        unmanaged_domain=None,
    ):
        self.type = type
        self.name = name
        self.url = url
        self.classic_url = classic_url
        self.managed_domain = managed_domain
        self.unmanaged_domain = unmanaged_domain

    def __str__(self):
        if self.is_dc():
            return f'DoubleCloud: {self.name}'
        elif self.is_compute():
            return f'YandexCloud: {self.name}'
        elif self.is_cloudil():
            return f'CloudIL: {self.name}'
        else:
            return f'{self.type.title()}: {self.name}'

    def __eq__(self, other):
        if self.name == other.name and self.type == other.type:
            return True
        else:
            return False

    @property
    def self_image(self):
        return TYPE_TO_IMAGE[self.type]

    @property
    def label(self):
        return f'{self.type.lower()}-{self.name.lower()}'

    def is_compute(self):
        return self.type == TYPE_COMPUTE

    def is_porto(self):
        return self.type == TYPE_PORTO

    def is_dc(self):
        return self.type == TYPE_DC

    def is_cloudil(self):
        return self.type == TYPE_CLOUDIL


INSTALLATIONS_MAP = {
    'porto-test': Installation(
        type=TYPE_PORTO,
        name='test',
        url=None,
        classic_url='https://ui.db.yandex-team.ru',
        managed_domain='db.yandex.net',
        unmanaged_domain='db.yandex.net',
    ),
    'porto-prod': Installation(
        type=TYPE_PORTO,
        name='prod',
        url=None,
        classic_url='https://ui-prod.db.yandex-team.ru',
        managed_domain='db.yandex.net',
        unmanaged_domain='db.yandex.net',
    ),
    'compute-preprod': Installation(
        type=TYPE_COMPUTE,
        name='preprod',
        url=None,
        classic_url='https://ui.db.yandex-team.ru',
        managed_domain='db.yandex.net',
        unmanaged_domain='mdb.cloud-preprod.yandex.net',
    ),
    'compute-prod': Installation(
        type=TYPE_COMPUTE,
        name='prod',
        url=None,
        classic_url='https://ui-compute-prod.db.yandex-team.ru',
        managed_domain='db.yandex.net',
        unmanaged_domain='mdb.yandexcloud.net',
    ),
    'dc-preprod': Installation(
        type=TYPE_DC,
        name='preprod',
        url='https://backstage.preprod.mdb.internal.yadc.io/',
        classic_url='https://ui-preprod.yadc.io',
        managed_domain='yadc.io',
        unmanaged_domain='yadc.io',
    ),
    'dc-prod': Installation(
        type=TYPE_DC,
        name='prod',
        url='https://backstage.prod.mdb.internal.double.tech/',
        classic_url='https://ui-prod.mdb.double.tech',
        managed_domain='at.double.cloud',
        unmanaged_domain='at.double.cloud',
    ),
    'cloudil-prod': Installation(
        type=TYPE_CLOUDIL,
        name='prod',
        url='https://backstage.mdb-cp.yandexcloud.co.il',
        classic_url=None,
        unmanaged_domain='mdb.cloudil.com',
        managed_domain='mdb.yandexcloud.co.il',
    ),
}
