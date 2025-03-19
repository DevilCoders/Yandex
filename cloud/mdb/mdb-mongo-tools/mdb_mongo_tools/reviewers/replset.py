"""
Replset predictor and replset checks module
"""

import logging
from abc import ABC, abstractmethod
from datetime import timedelta
from typing import Dict, Type

from mdb_mongo_tools.exceptions import (EtaTooLong, FreshSecondariesNotEnough, PredictorOperationNotAllowed,
                                        QuorumLoss)
from mdb_mongo_tools.mongodb import ReplSetInfo, ReplSetMemberStates

RS_INIT_SYNC_MULTIPLIER = 1.2


class BaseReplsetOpCheck(ABC):
    """
    Base check for replica set operations
    """

    def __init__(self, host_port: str, conf: dict, replset: ReplSetInfo):
        self._host_port = host_port
        self._conf = conf
        self._replset = replset

    def execute(self) -> None:
        """
        Perform check wrapper
        """

        logging.debug('Executing with config %s', self._conf)
        return self._execute()

    @abstractmethod
    def _execute(self) -> None:
        """
        Actual check method
        """


class QuorumCheck(BaseReplsetOpCheck):
    """
    Quorum check
    """

    def _execute(self) -> None:
        quorum = self._replset.votes_sum // 2 + 1
        current = self._replset.votes_sum - self._replset.unreach_votes_sum
        estimated = current - self._replset.get_host_votes(self._host_port)
        quorum_ok = estimated >= quorum
        logging.debug('Check estimated >= quorum: %s >= %s is %s, current votes is %s', estimated, quorum, quorum_ok,
                      current)
        if not quorum_ok:
            raise QuorumLoss('Quorum is going to be lost')


class WriteConcernCheck(BaseReplsetOpCheck):
    """
    Write concern check
    """

    def _execute(self):
        # TODO: check rs wc when current host is down
        # calc supported wc: majority, 1
        # filter hosts:
        #   - not data-bearing
        #   - replication lag > configured threshold
        # check if wc_after_resetup_run > configured_minimum
        # also check P-S-A configuration, mb it is not supported
        # see https://docs.mongodb.com/manual/reference/write-concern/#arbiters-majority-count
        pass


class EtaCheck(BaseReplsetOpCheck):
    """
    ETA of data fetch is acceptable
    """

    def _execute(self):
        time_to_sync = self._replset.primary_dbs_size // self._conf['net_bps']
        eta_ok = time_to_sync * RS_INIT_SYNC_MULTIPLIER < self._replset.rs_oplog_window
        logging.debug('Check resetup ETA estimated * multiplier < oplog window: %s * %s < %s is %s', time_to_sync,
                      RS_INIT_SYNC_MULTIPLIER, self._replset.rs_oplog_window, eta_ok)
        if not eta_ok:
            raise EtaTooLong('Estimated duration of resetup is too long')


class FreshSecondariesCheck(BaseReplsetOpCheck):
    """
    Secondaries replication lag is acceptable
    """

    def _execute(self):
        fresh_secondaries = []
        lag_seconds = timedelta(**self._conf['lag_threshold']).seconds
        fresh_secondaries_required = self._conf['hosts_required']
        for host_port in self._replset.members_host_port(state=ReplSetMemberStates.SECONDARY):
            if self._replset.get_host_lag(host_port) < lag_seconds:
                fresh_secondaries.append(host_port)

        fresh_secondaries_found = len(fresh_secondaries)
        fresh_secondaries_ok = fresh_secondaries_found >= fresh_secondaries_required
        logging.debug('Check fresh secondaries found >= required: %s >= %s is %s', fresh_secondaries_found,
                      fresh_secondaries_required, fresh_secondaries_ok)
        if not fresh_secondaries_ok:
            raise FreshSecondariesNotEnough('Up to date secondaries are not enough')


def get_replset_check_instance(rtype, *args, **kwargs) -> BaseReplsetOpCheck:
    """
    Return class instance for required type of service ctl
    """

    impl: Dict[str, Type[BaseReplsetOpCheck]] = {
        'quorum': QuorumCheck,
        'write_concern': WriteConcernCheck,
        'eta_acceptable': EtaCheck,
        'fresh_secondaries': FreshSecondariesCheck,
    }
    req_class = impl.get(rtype, None)

    if req_class:
        return req_class(*args, **kwargs)

    raise NotImplementedError('Unknown instance type: {rtype}'.format(rtype=rtype))


class ReplSetOpReviewer:
    """
    Operation with replset and its members checker
    """

    # TODO: think about refactoring:
    # we should work here with mutable copy of ReplSetInfo, let's say ReplSetDummy
    # e.g. replset_dummy = replset.get_dummy()
    # 1 shutdown_host() calls replset_dummy.shutdown_host(host_port)
    # 2 then run all our checks using replset_dummy
    # this provides flexible structure for future checks

    def __init__(self, resplset: ReplSetInfo, conf: dict):
        self._replset = resplset
        self._conf = conf

    def shutdown_host(self, host_port: str) -> None:
        """
        Check if host downtime is allowed
        """
        for check, opts in self._conf.items():
            if not opts['enabled']:
                logging.debug('Check "%s" is disabled', check)
                continue

            logging.debug('Running "%s" check', check)
            try:
                get_replset_check_instance(check, host_port, opts, self._replset).execute()
            except PredictorOperationNotAllowed:
                logging.warning('Check "%s" failed', check)
                if not opts['warn_only']:
                    raise

    def update(self) -> None:
        """
        Recollect replset status data
        """
        self._replset.update()
