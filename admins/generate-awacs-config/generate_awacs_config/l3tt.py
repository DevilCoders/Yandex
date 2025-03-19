from .api import API


class L3TT(API):
    API = 'https://l3.tt.yandex-team.ru/api/v1/'

    def find_service_id(self, name: str) -> int:
        """ Return a service id based on the name """
        return self._fetch('service?fqdn__exact=' + name)['objects'][0]['id']

    def find_service_groups(self, name: str) -> list:
        """ Return a service gencfg group list """
        self.log.info('Fetching %s gencfg groups from l3tt', name)

        service_id = self.find_service_id(name)
        result = self._fetch('service/%d' % service_id)['vs'][0]['group']
        return [x.split(':')[1].split('/')[0] for x in result]
