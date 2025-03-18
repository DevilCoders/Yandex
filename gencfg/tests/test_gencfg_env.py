"""Tests gencfg environment mock."""

from __future__ import unicode_literals

import os
import uuid

from core.svnapi import SvnRepository

import pytest

unstable_only = pytest.mark.unstable_only


@unstable_only
def unstable_gencfg_env(gencfg_env):
    value = str(uuid.uuid4())
    db_repo = SvnRepository(gencfg_env.db_path)

    with open(os.path.join(db_repo.path, value), "w") as f:
        f.write(value)

    db_repo.add([value])
    db_repo.commit(value, paths=[value])
