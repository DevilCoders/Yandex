"""Pillar modification logic."""
import logging

from .exceptions import UnsupportedClusterError
from .pillar import Pillar
from .mysql_schemas import MysqlPillarSchema
from .utils import IDM_ORIGIN, generate_password

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


INTERNAL_ROLES = {
    "reader": "mdb_reader",
    "writer": "mdb_writer",
}

INTERNAL_ROLES_REV = {
    "mdb_reader": "reader",
    "mdb_writer": "writer",
}


def default_user():
    """Default database user template."""
    return {
        # when we create user - we check connection, so pwd must not be empty
        'password': generate_password(),
        'origin': IDM_ORIGIN,
        'proxy_user': '',
        'last_password_update': 0,
    }


class MysqlPillar(Pillar):
    """An interface for Pillar modification."""

    def __init__(self, data):
        super().__init__(data)

    def validate(self):
        """Check that pillar matches the schema and has 'sox_audit' flag set."""
        validation_errors = MysqlPillarSchema().validate(self.data)
        if validation_errors:
            logger.error('Invalid cluster pillar %s', validation_errors)
            raise UnsupportedClusterError('Invalid cluster: {}'.format(validation_errors))
        if not self.data['data']['sox_audit']:
            logger.error('Pillar mush have "sox_audit" flag set')
            raise UnsupportedClusterError('Cluster is not under SOX audit')

    @property
    def users(self):
        """Get cluster users."""
        return self.data['data']['mysql']['users']

    @property
    def zk_hosts(self):
        """Get cluster zk_hosts."""
        return self.data['data']['mysql']['zk_hosts']

    def databases(self):
        """Get database names."""
        result = []
        for db in self.data['data']['mysql']['databases']:
            result.extend(db.keys())
        return result

    def get_default_user(self):
        return default_user()

    def add_user_role(self, user, role):
        user['proxy_user'] = self.get_internal_role(role)

    def remove_user_role(self, user, role):
        internal_role = self.get_internal_role(role)
        if user['proxy_user'] != internal_role:
            return False
        user['proxy_user'] = None

    def get_user_external_roles(self, user):
        """
        Get user roles for external system, or None
        """
        proxy_user = user.get('proxy_user', '')
        if not proxy_user:
            return None

        return [INTERNAL_ROLES_REV[proxy_user]]

    def get_internal_role(self, role):
        return INTERNAL_ROLES[role]
