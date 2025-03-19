"""Pillar modification logic."""
import logging

from .exceptions import UnsupportedClusterError
from .pillar import Pillar
from .pg_schemas import PgPillarSchema
from .utils import IDM_ORIGIN

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


GRANTS = [
    "reader",
    "writer",
]


def default_user(dbs=None):
    """Default database user template."""
    return {
        'password': '',
        'grants': [],
        'allow_db': '*',
        'connect_dbs': dbs or [],
        'bouncer': True,
        'create': True,
        'login': True,
        'replication': False,
        'superuser': False,
        'origin': IDM_ORIGIN,
        'conn_limit': 2,
        'allow_port': 6432,
        'last_password_update': 0,
    }


class PgPillar(Pillar):
    """An interface for Pillar modification."""

    def __init__(self, data):
        super().__init__(data)

    def validate(self):
        """Check that pillar matches the schema and has 'sox_audit' flag set."""
        validation_errors = PgPillarSchema().validate(self.data)
        if validation_errors:
            logger.error('Invalid cluster pillar %s', validation_errors)
            raise UnsupportedClusterError('Invalid cluster: {}'.format(validation_errors))
        if not self.data['data']['sox_audit']:
            logger.error('Pillar mush have "sox_audit" flag set')
            raise UnsupportedClusterError('Cluster is not under SOX audit')

    @property
    def users(self):
        """Get cluster users."""
        return self.data['data']['config']['pgusers']

    def databases(self):
        """Get database names."""
        result = []
        for db in self.data['data']['unmanaged_dbs']:
            result.extend(db.keys())
        return result

    def get_default_user(self):
        return default_user(dbs=self.databases())

    def add_user_role(self, user, role):
        user_roles = user['grants']
        if role in user_roles:
            return False
        user_roles.append(role)

    def remove_user_role(self, user, role):
        user_roles = user['grants']
        if role not in user_roles:
            return False
        user_roles.remove(role)

    def get_user_external_roles(self, user):
        """
        Get user roles for external system, or None
        """
        if 'grants' not in user:
            return None

        return [g for g in user['grants'] if g in GRANTS]
