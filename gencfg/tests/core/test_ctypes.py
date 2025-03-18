# coding=utf8

from tests.util import get_db_by_path

import core.ctypes


def test_unicode(testdb_path):
    CTYPE_NAME = 'custom_ctype'
    CTYPE_DESCR = 'Non-asscii описание ctype'

    try:
        db = get_db_by_path(testdb_path, cached=False)
        ctype = core.ctypes.CType(db.ctypes, {
            'name': CTYPE_NAME,
            'descr': CTYPE_DESCR,
        })
        db.ctypes.add_ctype(ctype)
        db.ctypes.update(smart=True)

        # check if we can read unicode data from ctype description
        newdb = get_db_by_path(testdb_path, cached=False)
        newdb.ctypes.get_ctype(CTYPE_NAME)
    finally:
        db.ctypes.remove_ctype(CTYPE_NAME)
        db.ctypes.update(smart=True)
