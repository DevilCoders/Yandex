# -*- coding: utf-8 -*-
import yaqutils.six_helpers as usix
from experiment_pool import MetricColoring


class SPUv2(object):
    def __init__(self):
        self.types = {"click", "request", "dynamic-click"}

    @staticmethod
    def split_actions(api):
        before_actions = []
        after_actions = []
        start_timestamp = api.observation.timestamp_from
        for action in api.actions:
            if action.timestamp < start_timestamp:
                before_actions.append(action)
            else:
                after_actions.append(action)
        return before_actions, after_actions

    def get_first_features(self, after_actions, api):
        first_ts = None
        for action in after_actions:
            if action.data.get("type") in self.types:
                first_ts = action.timestamp
                break
        if first_ts is None:
            return [0, 0]

        first_event = min(1000000, first_ts - api.observation.timestamp_from)
        key = api.user
        flash_age = 0
        if key.startswith("y") and len(key) > 10:
            flash_timestamp = int(key[10:])
            flash_age = min(1000000, max(0, first_ts - flash_timestamp))
        return [first_event, flash_age]

    def get_features(self, before_actions, after_actions, api):
        start_timestamp = api.observation.timestamp_from
        start = None
        previous = None
        sessions = [0] * 14
        for action in before_actions:
            if action.data.get("type") in self.types:
                if previous is None:
                    previous = action.timestamp
                    start = action.timestamp
                if action.timestamp - previous > 30 * 60:
                    prev_day = (start_timestamp - start) / 86400
                    if prev_day < len(sessions):
                        sessions[prev_day] += 1
                    start = action.timestamp
                previous = action.timestamp
        if previous is not None:
            prev_day = (start_timestamp - start) / 86400
            if prev_day < len(sessions):
                sessions[prev_day] += 1
        sessions_extended = sessions
        sessions_sum = 0
        for i in usix.xrange(0, 14):
            sessions_sum += sessions[i]
            sessions_extended.append(sessions_sum)
        first_features = self.get_first_features(after_actions, api)
        return first_features + sessions_extended

    def __call__(self, api):
        before_actions, after_actions = SPUv2.split_actions(api)
        features = self.get_features(before_actions, after_actions, api)
        sessions = 0
        previous = None
        for action in after_actions:
            if action.data.get("type") in self.types:
                current = action.timestamp
                if previous is None or current - previous > 30 * 60:
                    sessions += 1
                previous = current
        if sessions:
            return {"target": sessions, "values": features}

    coloring = MetricColoring.MORE_IS_BETTER
