from .api import API


class Gencfg(API):
    API = 'https://api.gencfg.yandex-team.ru/'

    def get_group_port(self, group: str) -> int:
        self.log.info('Resolving %s port', group)

        items = self._fetch('trunk/groups/%s/instances' % group)['instances']
        for item in items:
            return item['port']
        raise RuntimeError('Cannot determine %s port' % group)
