"""
MongoDB getters module
"""

from abc import ABC, abstractmethod
from enum import Enum
from typing import Dict

from mdb_mongo_tools.exceptions import (GetterFailure, GetterUnexpectedError, PredictorOperationNotAllowed,
                                        ReplSetInfoGatheringError, ReplSetInfoMultiplePrimariesFound,
                                        ReplSetInfoPrimaryNotFound)
from mdb_mongo_tools.mongodb import MongoConnection, ReplSetInfo
from mdb_mongo_tools.reviewers.replset import ReplSetOpReviewer


class MdbMongoGetters(Enum):
    """
    Mongo getter enums
    """
    IS_ALIVE = 'is_alive'
    IS_HA = 'is_ha'
    IS_PRIMARY = 'is_primary'
    PRIMARY_EXISTS = 'primary_exists'
    SHUTDOWN_ALLOWED = 'shutdown_allowed'


class BaseGetter(ABC):
    """
    Base getter class
    """

    @abstractmethod
    def execute(self) -> str:
        """
        Execute getter
        """


class IsAliveGetter(BaseGetter):
    """
    Check if mongo is alive
    """

    def __init__(self, conn: MongoConnection):
        self._conn = conn

    def execute(self):
        if not self._conn.is_alive:
            raise GetterFailure('Could not connect to mongodb')
        return "mongodb is alive"


class PrimaryExistsGetter(BaseGetter):
    """
    Check if primary is present in replset
    """

    def __init__(self, replset_info: ReplSetInfo):
        self._replset_info = replset_info

    def execute(self):
        try:
            self._replset_info.validate()
        except (ReplSetInfoMultiplePrimariesFound, ReplSetInfoPrimaryNotFound) as exc:
            raise GetterFailure(exc)
        return "One primary exists in replica set"


class IsPrimaryGetter(BaseGetter):
    """
    Check if mongo is primary
    """

    def __init__(self, replset_info: ReplSetInfo, host_port: str):
        self._replset_info = replset_info
        self._host_port = host_port

    def execute(self):
        try:
            if self._host_port != self._replset_info.primary_host_port:
                raise GetterFailure('{hp} is not primary'.format(hp=self._host_port))
        except ReplSetInfoGatheringError as exc:
            raise GetterUnexpectedError(exc)
        return "{hp} is primary".format(hp=self._host_port)


class ReplicaSetIsHaGetter(BaseGetter):
    """
    Check if replset configuration provides high availability
    """

    def __init__(self, replset_info: ReplSetInfo):
        self._replset_info = replset_info

    def execute(self):
        if self._replset_info.votes_sum < 3:
            raise GetterFailure(
                'Not enough votes in replset config: {votes} < 3'.format(votes=self._replset_info.votes_sum))
        return "Replica set config has more than 3 votes"


class HostShutdownAllowedGetter(BaseGetter):
    """
    Check if given host shutdown allowed
    """

    def __init__(self, rs_op_reviewer: ReplSetOpReviewer, host_port: str):
        self._rs_op_reviewer = rs_op_reviewer
        self._host_port = host_port

    def execute(self):
        try:
            self._rs_op_reviewer.shutdown_host(self._host_port)
            return "{hp} shutdown is allowed".format(hp=self._host_port)
        except PredictorOperationNotAllowed:
            raise GetterFailure('{hp} shutdown is not allowed'.format(hp=self._host_port))


class GetterExc:
    """
    Run given getter
    """

    def __init__(self, conf):
        self._conn_conf = conf['mongodb']
        self._getter_conf = conf['getter']
        self._conn = MongoConnection(self._conn_conf)
        self._replset_info = ReplSetInfo(self._conn_conf, self._conn)
        self._rs_op_reviewer = None
        self._host_port = '{host}:{port}'.format(**conf['mongodb'])
        self._getters: Dict[MdbMongoGetters, BaseGetter] = {}

    def _load_getters(self):
        self._replset_info.update(strict=False)
        self._rs_op_reviewer = ReplSetOpReviewer(self._replset_info, self._getter_conf['replset_checks'])

        self._getters[MdbMongoGetters.PRIMARY_EXISTS] = PrimaryExistsGetter(self._replset_info)
        self._getters[MdbMongoGetters.IS_ALIVE] = IsAliveGetter(self._conn)
        self._getters[MdbMongoGetters.IS_HA] = ReplicaSetIsHaGetter(self._replset_info)
        self._getters[MdbMongoGetters.IS_PRIMARY] = IsPrimaryGetter(self._replset_info, self._host_port)
        self._getters[MdbMongoGetters.SHUTDOWN_ALLOWED] = HostShutdownAllowedGetter(
            self._rs_op_reviewer, self._host_port)

    def perform(self, getter: MdbMongoGetters) -> str:
        """
        Run command
        """

        # ugly code alert: we have main getter that relies on alive mongod, consider:
        # TODO refactoring: build dependencies between getters and execute in right order up to required getter

        try:
            is_alive = IsAliveGetter(self._conn).execute()
            if getter == MdbMongoGetters.IS_ALIVE:
                return is_alive
        except GetterFailure as exc:
            if getter == MdbMongoGetters.IS_ALIVE:
                raise
            raise GetterUnexpectedError(exc)

        self._load_getters()

        return self._getters[getter].execute()
