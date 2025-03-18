# coding=utf8

from tests.util import get_db_by_path

import core.itypes


def test_unicode(testdb_path):
    ITYPE_NAME = 'custom_itype'
    ITYPE_DESCR = 'Non-asscii описание itype'

    try:
        db = get_db_by_path(testdb_path, cached=False)
        itype = core.itypes.IType(db.itypes, {
            'name': ITYPE_NAME,
            'descr': ITYPE_DESCR,
        })
        db.itypes.add_itype(itype)
        db.itypes.update(smart=True)

        # check if we can read unicode data from itype description
        newdb = get_db_by_path(testdb_path, cached=False)
        newdb.itypes.get_itype(ITYPE_NAME)
    finally:
        db.itypes.remove_itype(ITYPE_NAME)
        db.itypes.update(smart=True)
