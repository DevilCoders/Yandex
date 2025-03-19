import re
import time
import logging

from .types import Update, Message  # noqa
import typing as t  # noqa

import six

logger = logging.getLogger('ya-messenger')


class Dispatcher(object):

    def __init__(self, sleep_time=1.0):
        # type: (float) -> None
        self._sleep_time = sleep_time
        self._offset = 0  # type: int
        self._handlers = []  # type: t.List[t.Tuple[t.Callable[[Message], None], t.Dict[six.text_type, t.Any]]]

    @staticmethod
    def _check_message(message, checks):
        # type: (Message, t.Dict[six.text_type, t.Any]) -> bool
        # count of successful checks
        cnt = 0
        if 'content_type' in checks:
            if ('document' in checks['content_type'] and message.document) or \
               ('text' in checks['content_type'] and message.text and not message.gallery) or \
               ('photo' in checks['content_type'] and message.photo) or \
               ('gallery' in checks['content_type'] and message.gallery) or \
               ('sticker' in checks['content_type'] and message.sticker):
                cnt += 1

        if message.text and 'regexp' in checks and checks['regexp'].search(message.text) is not None:
            cnt += 1

        if 'func' in checks and checks['func'](message):
            cnt += 1

        if message.text in checks.get('commands', []):
            cnt += 1

        return cnt == len(checks)

    def serve_message(self, message):
        # type: (Message) -> None
        for func, checks in self._handlers:
            if self._check_message(message, checks):
                func(message)
                break
        else:
            logger.info("No handler matched for message with id={}".format(message.id))

    def serve_update(self, update):
        # type: (Update) -> None
        self.serve_message(update.message)

    def polling(self):
        # type: () -> None
        logger.info('Start polling...')
        while True:
            try:
                updates = self.get_updates(self._offset)
                for update in updates:
                    self.serve_update(update)

                if len(updates) != 0:
                    self._offset = updates[-1].id + 1

            except Exception as e:
                logger.error(e, exc_info=e)
            finally:
                time.sleep(self._sleep_time)

    @staticmethod
    def _check_list_of_strings(
        lst,  # type: t.Iterable[t.Any]
        name,  # type: six.text_type
        allowed_values=None  # type: t.Optional[t.Set[six.text_type]]
    ):
        # type: (...) -> None
        if not isinstance(lst, (list, tuple)):
            raise ValueError('{} should be a list or tuple of string'.format(name))

        for i in lst:
            if not isinstance(i, six.string_types) or (allowed_values is not None and i not in allowed_values):
                raise ValueError("{} element should be one of {}".format(name, list(allowed_values)))  # type: ignore

    def message_handler(self, **kwargs):
        # type: (t.Dict[six.text_type, t.Any]) -> t.Callable[[t.Callable[[Message], None]], t.Callable[[Message], None]]
        """
        :param kwargs:
            func=lambda message: True  func takes predicate and call handler if it's true
            regexp=r'[0-9]+'  regexp takes regular expression and call handler if text message came and match is true
            commands=['start']  commands takes list of commands if message text is /{command} handler will be called
            content_type=['photo', 'document', 'text']  content_type takes list of types (only this three are supported)
        :return:
        """
        checks = {'func', 'regexp', 'commands', 'content_type'}

        def deco(f):
            # type: (t.Callable[[Message], None]) -> t.Callable[[Message], None]
            for check in kwargs:
                if check not in checks:
                    raise ValueError('Unexpected argument: {}'.format(check))

            if 'content_type' in kwargs:
                self._check_list_of_strings(kwargs['content_type'], 'content_type', {'document', 'photo', 'text', 'gallery', 'sticker'})

            if 'commands' in kwargs:
                self._check_list_of_strings(kwargs['commands'], 'commands')
                kwargs['commands'] = ['/' + cmd for cmd in kwargs['commands']]  # type: ignore

            if 'func' in kwargs and not callable(kwargs['func']):
                raise ValueError('func should be callable')

            if 'regexp' in kwargs:
                if not isinstance(kwargs['regexp'], six.string_types):
                    raise ValueError('regexp should be string')
                kwargs['regexp'] = re.compile(kwargs['regexp'])

            self._handlers.append((f, kwargs))
            return f
        return deco

    def get_updates(self, offset):
        # type: (int) -> t.List[Update]
        raise NotImplementedError
