# -*- coding: utf-8 -*-

import copy
import json
import logging

import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
from metrics_api import ExperimentForAPI  # noqa
from metrics_api import ObservationForAPI  # noqa


# NOTICE: Classes below are used directly in user metrics.
# Interface/fields should NOT be changed/renamed/etc. without 'mstand community' approval.


class UserActionsForAPI(object):
    def __init__(self, user, actions, experiment, observation, history=None, future=None):
        """
        :type user: str
        :type actions: list[UserActionForAPI]
        :type history: list[UserActionForAPI]
        :type future: list[UserActionForAPI]
        :type experiment: ExperimentForAPI
        :type observation: ObservationForAPI
        """
        self.user = user
        self.actions = actions
        self.history = history or []
        self.future = future or []
        self.experiment = experiment
        self.observation = observation

    def __iter__(self):
        return iter(self.actions)

    def dump(self):
        action_list = umisc.to_lines(self.actions)
        history_list = umisc.to_lines(self.history)
        future_list = umisc.to_lines(self.future)

        template = u"UserActionsForAPI\n{}\n{}\nUser: {}\nActions: {}\nHistory: {}\nFuture: {}"
        return template.format(self.experiment, self.observation, self.user, action_list, history_list, future_list)

    def log(self):
        logging.warning(self.dump())

    @staticmethod
    def header_from_other(actions_object):
        return UserActionsForAPI(
            user=actions_object.user,
            experiment=actions_object.experiment,
            observation=actions_object.observation,
            history=actions_object.history,
            future=actions_object.future,
            actions=[],
        )

    def clone(self):
        experiment = copy.copy(self.experiment)
        observation = copy.copy(self.observation)
        actions = [a.clone() for a in self.actions]
        history = [a.clone() for a in self.history]
        future = [a.clone() for a in self.future]
        return UserActionsForAPI(
            user=self.user,
            actions=actions,
            experiment=experiment,
            observation=observation,
            history=history,
            future=future,
        )

    def apply_filter(self, user_filter):
        if user_filter is None:
            return self
        clone = self.clone()
        clone.actions = user_filter(clone.actions)
        if clone.actions:
            return clone
        else:
            return None


UTF_SQUEEZE_FIELDS = ("query", "correctedquery", "suggestion", "suggest")


def fix_utf_fields(data):
    """
    :type data: dict
    """
    for field_name in UTF_SQUEEZE_FIELDS:
        if field_name in data:
            field_value = data.get(field_name)
            data[field_name] = umisc.to_unicode_recursive(field_value)


class UserActionForAPI(object):
    def __init__(self, user, timestamp, data):
        """
        :type user: str
        :type timestamp: str | int
        :type data: dict
        """
        self.user = user
        self.timestamp = int(timestamp)
        self.datetime = utime.timestamp_to_datetime_msk(self.timestamp)
        self.date = self.datetime.date()
        self.data = data
        fix_utf_fields(self.data)

    def __str__(self):
        return self.dump()

    def dump(self):
        try:
            data = json.dumps(self.data, indent=2, sort_keys=True, ensure_ascii=False)
        except UnicodeDecodeError:
            logging.error("Cannot decode the dump into UTF-8, dumping it as is")
            return repr(self.data)
        else:
            return u"UserActionForAPI\nDatetime: {}\nData: {}".format(self.datetime, data)

    def log(self):
        logging.warning(self.dump())

    def get(self, name, default=None):
        return self.data.get(name, default)

    @staticmethod
    def from_row(row):
        """
        :type row: dict
        :rtype: UserActionForAPI
        """
        return UserActionForAPI(row["yuid"], row["ts"], row)

    def clone(self):
        data = copy.deepcopy(self.data)
        return UserActionForAPI(
            user=self.user,
            timestamp=self.timestamp,
            data=data,
        )
