"""Test getting documentation calls"""

import os

import pytest

unstable_only = pytest.mark.unstable_only

from tests.util import get_db_by_path

from core.card.node import TMdDoc
from config import SCHEME_LEAF_DOC_FILE


def test_get_mddoc(wbe):
    TO_TEST = [
        ("group.yaml", "name"),
        ("group.yaml", "reqs.instances.memory_guarantee"),
    ]

    db = get_db_by_path(wbe.db_path, cached=False)
    db_mddoc = TMdDoc(os.path.join(db.SCHEMES_DIR, SCHEME_LEAF_DOC_FILE))

    for fname, path in TO_TEST:
        result = wbe.api.get("/unstable/mddoc/%s/%s" % (fname, path))
        assert result["operation success"] == True, "Getting doc from file <%s> with path <%s> failed" % (fname, path)
        assert result["doc"].encode("utf-8") == db_mddoc.get_doc(fname, path, TMdDoc.EFormat.HTML)


def test_get_mddoc_fail(wbe):
    result = wbe.api.get("/unstable/mddoc/group.yaml/nonexisting.field")
    assert result["operation success"] == False
