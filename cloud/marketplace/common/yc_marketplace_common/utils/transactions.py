import functools

from yc_common import config
from yc_common.clients.kikimr import TransactionMode
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.utils.errors import ReadOnlyError


def mkt_transaction(tx_mode: TransactionMode = TransactionMode.SERIALIZABLE_READ_WRITE):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, tx=None, **kwargs):
            if tx is not None:
                return func(*args, tx=tx, **kwargs)

            read_only = config.get_value("marketplace.read_only", False)
            if read_only and tx_mode == TransactionMode.SERIALIZABLE_READ_WRITE:
                raise ReadOnlyError()

            # with marketplace_db().transaction(tx_mode=tx_mode) as transaction:
            with marketplace_db().transaction() as transaction:  # Downgrade
                return func(*args, tx=transaction, **kwargs)
        return wrapper
    return decorator
