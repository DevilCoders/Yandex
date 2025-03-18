from typing import Dict, List

from .base import ApiResource


class Reference(ApiResource):
    """Базовые справочники."""

    def get_associates(self) -> Dict[str, dict]:
        """Возвращает данные о внешних системах (банках и платёжных системах),
        с которыми умеет работать BCL. Словарь индексирован псевдонимами внешних систем.

        Пример:
            {
                ...
                'yad': {'alias': 'yad', 'title': 'Яндекс.Деньги'},
                ...
            }

        """
        items = self._conn.request(url='refs/associates/').data.get('items', [])
        return {item['alias']: item for item in items}

    def get_services(self) -> Dict[str, dict]:
        """Возвращает данные о сервисах Яндекса, с которыми умеет работать BCL.
        Словарь индексирован псевдонимами сервисов.

        Пример:
            {
                ...
                'qa': {'alias': 'qa', 'title': 'Тестирование BCL', 'tvm_app': '12345'},
                ...
            }

        """
        items = self._conn.request(url='refs/services/').data.get('items', [])
        return {item['alias']: item for item in items}

    def get_statuses(self) -> Dict[str, List[dict]]:
        """Возвращает данные о сатусах/состояниях, которые использует BCL
        для разных сущностей (платежей, выписок). Словарь индексирован псевдонимами типов сущностей.

        Пример:
            {
                'payments': [...],
                'bundles': [...],
                'statements': [
                    {'alias': 'new', 'title': 'Новый'},
                    ...
                ]
                ...
            }

        """
        items = self._conn.request(url='refs/statuses/').data.get('items', [])
        return {item['realm']: item['statuses'] for item in items}

    def get_accounts(self, numbers: List[str]) -> List[dict]:
        """Возвращает данные о зарегистрированных в BCL счетах.

        Пример:
            [{'number': '40702810538000111471', 'blocked': 0}]

        :param numbers: Список идентификаторов (номеров) счетов,
            для которых требуется получить информацию.

        """
        items = self._conn.request(url='refs/accounts/', data={
            'accounts': numbers,
        }).data.get('items', [])

        return items
