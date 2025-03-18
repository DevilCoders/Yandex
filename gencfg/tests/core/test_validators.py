"""
    Test group card validators to work correctly
"""

import copy

from core.exceptions import TValidateCardNodeError

from tests.util import get_db_by_path

def test_owners_validator(testdb_path):
    try:
        db = get_db_by_path(testdb_path, cached=False)

        group = db.groups.get_group('TEST_VALIDATE_OWNERS')
        vm_group = db.groups.get_group('TEST_VALIDATE_OWNERS_GUEST')

        initial_owners = copy.copy(group.card.owners)

        # check that we can add common user
        group.card.owners.append('osol')
        group.mark_as_modified()
        db.groups.update(smart = True)

        # check that we can not add robot user to virtual only groups
        group.card.owners.append('robot-gencfg')
        group.mark_as_modified()
        try:
            db.groups.update(smart = True)
        except TValidateCardNodeError:
            group.card.owners = initial_owners

        # check that we can add robot user to virtual hots group
        vm_group.card.owners.append('robot-gencfg')
        vm_group.mark_as_modified()
        db.groups.update(smart = True)

        # check that we can not add unexisting user
        group.card.owners.append('some_unexisting_user')
        group.mark_as_modified()
        try:
            db.groups.update(smart = True)
        except TValidateCardNodeError:
            group.owners = initial_owners
    finally:
        try:
            testdb = get_db_by_path(testdb_path, cached=False)
            testdb.get_repo().reset_all_changes(remove_unversioned=True)
        except:
            pass
