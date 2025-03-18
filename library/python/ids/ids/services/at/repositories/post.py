# coding: utf-8

from six.moves.urllib.parse import parse_qs, urlparse
from lxml import etree
from lxml.builder import E, ElementMaker

from .base import AtBaseRepository
from .utils import is_club_id

from ids.registry import registry
from ids.storages.null import NullStorage


class PostRepository(AtBaseRepository):
    """
    Репозиторий "Посты"

    """
    SERVICE = 'at'
    RESOURCES = 'post'

    def get_posts(self, resource, url_vars, params=None):
        data = self.connector.get(resource, url_vars=url_vars,
                                  params=params or {})

        for post in data['entries']:
            yield post

        if 'next' in data['links']:
            query = parse_qs(urlparse(data['links']['next']).query)
            for post in self.get_posts(resource, url_vars, query):
                yield post

    def search(self, lookup):
        """
        Получить итератор постов
        Если указан конкретный номер поста, то будет не больше одного результата.

        lookup = {'uid': 1234567, 'post_no': None, 'post_type': 'text'}
        выберет текстовые посты у пользователя/клуба 1234567

        """
        uid = lookup['uid']
        post_no = lookup.get('post_no')
        post_type = lookup.get('post_type')

        if is_club_id(uid):
            resource = 'club_'
        else:
            resource = 'person_'

        if post_no is not None:
            resource += 'post'
            data = self.connector.get(resource,
                                      url_vars={'uid': uid, 'post_no': post_no})
            yield data
            return

        elif post_type is not None:
            resource += 'typed_posts'
        else:
            resource += 'posts'

        for post in self.get_posts(
                resource, url_vars={'uid': uid, 'post_type': post_type}):
            yield post

    def create_(self, fields):
        """ Поддерживает пока только создание постов-ссылок
        fields - dict с полями:

        'uid'        - uid пользователя или клуба
        'access'     - public (default) / private
        'type'       - тип поста (пока поддерживается только `link`)
        'title'      - заголовок поста
        'content'    - контент поста
        'url'        - урл для шаринга (специфично для `type=link`)
        'store_time' - время создания поста (нужно чтобы создавать задним числом)
                       необязательный параметр, timestamp (int)

        """
        uid = fields['uid']

        if is_club_id(uid):
            resource = 'club_'
        else:
            resource = 'person_'

        resource += 'posts'

        if fields['type'] != 'link':
            raise NotImplementedError('Only `link` post type is supported')

        Y_URI = 'http://api.yandex.ru/yaru/'
        Y_NS = '{' + Y_URI + '}'

        el = ElementMaker(nsmap={'y': Y_URI,
                                 None: "http://www.w3.org/2005/Atom"})
        data = (
            el.entry(
                E.title(fields['title']),
                el(Y_NS + 'access', fields.get('access', 'public')),
                E.category(scheme="urn:ya.ru:posttypes", term=fields['type']),
                el(Y_NS + 'meta',
                   el(Y_NS + 'url', fields['url'])),
                E.content(fields['content'])
            )
        )

        if fields.get('store_time') is not None:
            data.append(el(Y_NS + 'store_time', fields['store_time']))

        self.connector.post(
            resource, url_vars={'uid': uid},
            headers={'Content-Type': ('application/atom+xml; '
                                      'type=entry; charset=utf-8')},
            data=etree.tostring(data, pretty_print=True, xml_declaration=True,
                                encoding='utf-8'))


def factory(**options):
    storage = NullStorage()
    repository = PostRepository(storage, **options)
    return repository


registry.add_repository(
    PostRepository.SERVICE,
    PostRepository.RESOURCES,
    factory,
)
