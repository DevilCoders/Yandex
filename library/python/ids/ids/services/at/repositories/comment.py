# coding: utf-8
from six import moves
from .base import AtBaseRepository
from .utils import is_club_id

from ids.registry import registry
from ids.storages.null import NullStorage


class CommentRepository(AtBaseRepository):
    """
    Репозиторий "Комментарий"

    """
    SERVICE = 'at'
    RESOURCES = 'comment'

    def search(self, lookup):
        """
        lookup = {'uid': 123, 'post_no': 321, 'comment_id': None}
        Комментарии поста пользователя 123, пост с номером 321

        lookup = {'uid': 123, 'post_no': 321, 'comment_id': 1}
        Один комментарий с id=1
        """
        uid = lookup['uid']
        post_no = lookup['post_no']

        resource = '%s_comments' % ('club' if is_club_id(uid) else 'person')

        data = self.connector.get(resource,
                                  url_vars={'uid': uid, 'post_no': post_no})

        comments = self._flat_comments(data['replies'])

        # чтобы выбрать конкретный комментарий, пробегаемся по всем
        if lookup.get('comment_id'):
            id_ = 'urn:ya.ru:comment/{uid}/{post_no}/{comment_id}'.format(**lookup)
            comments = moves.filter(lambda x: x['id'] == id_, comments)

        return comments

    def _flat_comments(self, comments):
        for comment in comments:
            replies = comment.pop('replies', [])
            yield comment

            for i in self._flat_comments(replies):
                yield i


def factory(**options):
    storage = NullStorage()
    repository = CommentRepository(storage, **options)
    return repository


registry.add_repository(
    CommentRepository.SERVICE,
    CommentRepository.RESOURCES,
    factory,
)
