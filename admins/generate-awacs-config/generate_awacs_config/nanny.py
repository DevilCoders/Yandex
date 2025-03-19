from .api import API


class Nanny(API):
    API = 'https://nanny.yandex-team.ru/v2/'

    def find_services_with_groups(self, groups: list) -> list:
        """ Find services with specified gencfg groups """
        self.log.info('Finding matching nanny services')

        groups = set(groups)
        items = []
        skip = 0
        while True:
            item_page = self._fetch('services/?category=/balancer&limit=200&skip=%s' % skip)['result']
            items = items + item_page
            skip += 200
            if len(item_page) < 200:
                break
        result = []
        for item in items:
            try:
                item_groups = set([x['name'] for x in item['runtime_attrs']['content']['instances']['extended_gencfg_groups']['groups']])
            except KeyError:
                continue
            if item_groups.intersection(groups):
                result.append(item['_id'])
        return result
