class SchemaNotInitializedError(Exception):
    pass


class SessionNotInitializedError(Exception):
    pass


class Context(object):
    _schema = None
    _session = None

    @property
    def schema(self):
        if self._schema is None:
            raise SchemaNotInitializedError("The schema must be first initialized")
        else:
            return self._schema

    @schema.setter
    def schema(self, schema):
        self._schema = schema

    @property
    def session(self):
        if self._session is None:
            raise SessionNotInitializedError("The session must be first initialized")
        else:
            return self._session

    @session.setter
    def session(self, session):
        self._session = session

