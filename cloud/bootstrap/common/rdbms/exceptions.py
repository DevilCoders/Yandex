from ..exceptions import BootstrapError


class DbError(BootstrapError):
    pass


class RecordNotFoundError(DbError):
    """Raised when selecting record and record is not found"""
    pass


class RecordAlreadyInDbError(DbError):
    """Raised when inserting new record but record with same id/name is already in db"""
    pass


class MultipleRecordsFoundError(DbError):
    pass
