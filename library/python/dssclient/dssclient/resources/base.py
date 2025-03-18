from typing import Optional, Union, List, Any

if False:  # pragma: nocover
    from ..endpoints.base import Endpoint  # noqa


class Resource:
    """База для ресурсов, получаемых по API."""

    _data: Optional[dict] = None
    """Данные, полученные с сервера."""

    _endpoint: 'Endpoint' = None
    """Объект конечной точки. Населяется при помощи _link_to_endpoint()."""

    _url_base: str = None
    """Базовый для типа ресурса URL."""

    def __init__(self, id: int = None):
        self.id: int = id

    def __str__(self):
        return f'ID: {self.id}'

    @classmethod
    def spawn(cls, data: Union[dict, int], endpoint: 'Endpoint') -> 'Resource':
        """Альтернативный конструктор. Создаёт экземпляр
        и инициализирует сырыми данными [с сервера].

        :param data: Словарь с данными, либо идентификатор сущности.
        :param endpoint:

        """
        is_dict = isinstance(data, dict)

        obj = cls(id=None if is_dict else data)
        obj._link_to_endpoint(endpoint)

        is_dict and obj._set_data(data)

        return obj

    def asdict(self) -> dict:
        """Представляет ресурс в виде словаря."""

        if self._data is None:
            self.refresh()

        return self._data

    def pformat(self, data:  dict = None, indent: int = 4) -> Union[str, List[str]]:
        """Форматирует данные, представляя их в удобочитаемом виде.

        :param data: Данные, которые требуется отформатировать.
        :param indent: Ширина отступа слева (количество пробелов).

        """
        do_join = False

        if data is None:
            data = self.asdict()
            do_join = True

        indent_str = ' ' * indent

        def add_line(line):
            lines.append(f'{indent_str}{line}')

        lines = []

        if isinstance(data, list):
            for val in sorted(data):
                lines.extend(self.pformat(val, indent))
                lines.append('')

        elif isinstance(data, dict):
            for key, val in sorted(data.items()):
                if isinstance(val, (dict, list)):
                    add_line(key + ':')
                    lines.extend(self.pformat(val, indent + 4))

                else:
                    add_line(f'{key}: {val}')
        else:
            add_line(data)

        if do_join:
            return '\n'.join(lines)

        return lines

    def _set_data(self, data: dict):
        """Населяет объект сырыми данными (полученными ранее с сервера)."""
        self._data = data
        self.id = data.get('ID', None)

    def _link_to_endpoint(self, endpoint: 'Endpoint'):
        """Устанавливает связь с объектом ключевой точки."""
        self._endpoint = endpoint

    def _get_data_item(self, key: str) -> Any:
        """Возвращает данные, полученные с сервера, адресуя именем атрибута данного объекта."""

        if self._data is None:
            self.refresh()

        return self._data[key]

    def _get_entity_url(self, postfix: str = '') -> str:
        url = f'{self._url_base}/{self.id}'

        if postfix:
            url = f'{url}/{postfix}'

        return url

    def refresh(self) -> 'Resource':
        """Запрашивает данные ресурса с сервера. И обновляет ими текущий объект."""
        data = self._endpoint._call(self._get_entity_url())
        self._set_data(data)
        return self
