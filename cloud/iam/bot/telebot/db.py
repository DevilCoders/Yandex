import ydb


class Db:
    def __init__(self, config):
        self._driver = ydb.Driver(endpoint=config['endpoint'], database=config['database'])
        self._driver.wait(fail_fast=True, timeout=config['timeout'])
        self._request_settings = ydb.BaseRequestSettings().with_timeout(config['timeout']).with_operation_timeout(config['timeout'])
        self._pool = ydb.SessionPool(self._driver)

    def list_chats(self):
        result = self._pool.retry_operation_sync(self._list_chats)
        return result[0].rows

    def create_chat(self, chat_id):
        self._pool.retry_operation_sync(lambda session: self._create_chat(session, chat_id))

    def delete_chat(self, chat_id):
        self._pool.retry_operation_sync(lambda session: self._delete_chat(session, chat_id))

    def find_chat(self, chat_id):
        result = self._pool.retry_operation_sync(lambda session: self._find_chat(session, chat_id))
        return result[0].rows[0]

    def _list_chats(self, session):
        return session.transaction().execute(
            'SELECT * FROM `chats` ORDER BY chat_id',
            commit_tx=True,
            settings=self._request_settings
        )

    def _create_chat(self, session, chat_id):
        query = session.prepare('DECLARE $chat_id AS Int64; INSERT INTO `chats` (chat_id) VALUES ($chat_id)')
        return session.transaction().execute(
            query,
            {'$chat_id': chat_id},
            commit_tx=True,
            settings=self._request_settings
        )

    def _delete_chat(self, session, chat_id):
        query = session.prepare('DECLARE $chat_id AS Int64; DELETE FROM `chats` WHERE chat_id = $chat_id')
        return session.transaction().execute(
            query,
            {'$chat_id': chat_id},
            commit_tx=True,
            settings=self._request_settings
        )

    def _find_chat(self, session, chat_id):
        query = session.prepare('DECLARE $chat_id AS Int64; SELECT * FROM `chats` WHERE chat_id = $chat_id')
        return session.transaction().execute(
            query,
            {'$chat_id': chat_id},
            commit_tx=True,
            settings=self._request_settings
        )
