# -*- coding: utf-8 -*-
import json
import logging
import os.path

import yt.wrapper as yt

from library.python.monitoring.solo.controller.resource import ResourceAction, Resource

logger = logging.getLogger(__name__)


class State(object):
    def __init__(
        self,
        save,
        delete_untracked
    ):
        self._data = None
        self._save = save
        self._delete_untracked = delete_untracked

    def default_data(self):
        return {"resources": {}}

    def get_resources_provider_id(self, resources):
        for resource in resources:
            if resource.local_id in self._data["resources"]:
                resource.provider_id = self._data["resources"][resource.local_id]

    def get_untracked_resources(self, resources):
        if not self._delete_untracked:
            return []
        untracked_resources = []
        tracked_resource_ids = {resource.local_id for resource in resources}
        for local_id in self._data["resources"]:
            if local_id not in tracked_resource_ids:
                untracked_resources.append(Resource(
                    local_id=local_id,
                    local_state=None,
                    provider_id=self._data["resources"][local_id],
                    provider_state=None
                ))
        return untracked_resources

    def set_resources_provider_id(self, resources):
        for resource in resources:
            if resource.action in {ResourceAction.CREATE, ResourceAction.MODIFY, ResourceAction.SKIP}:
                self._data["resources"][resource.local_id] = resource.provider_id
            elif resource.action in {ResourceAction.DELETE, ResourceAction.IGNORE}:
                self._data["resources"].pop(resource.local_id, None)
        self.save()

    def __enter__(self):
        raise NotImplementedError()

    def __exit__(self, error_type, error_value, error_traceback):
        raise NotImplementedError()

    def save(self):
        raise NotImplementedError()


class LocalFileState(State):
    def __init__(self, save, delete_untracked, state_file_path):
        super(LocalFileState, self).__init__(save, delete_untracked)
        self.state_file_path = state_file_path

    def __enter__(self):
        if os.path.exists(self.state_file_path):
            with open(self.state_file_path, "r") as local_file:
                self._data = json.load(local_file)
        else:
            logger.info("Local state file \"{0}\" does not exist, using default state".format(self.state_file_path))
            self._data = self.default_data()

    def __exit__(self, error_type, error_value, error_traceback):
        # do not save on any errors
        if error_value:
            logger.info("Error during resources processing, state will not be modified for this objects group!")

    def save(self):
        if not self._save:
            return
        with open(self.state_file_path, "w") as local_file:
            logger.debug("Saving state to local state file \"{0}\"".format(self.state_file_path))
            json.dump(self._data, local_file, indent=4, sort_keys=True)


class LockeNodeState(State):
    def __init__(self, save, delete_untracked, state_yt_cluster, state_locke_path, state_locke_token):
        super(LockeNodeState, self).__init__(save, delete_untracked)
        self.state_yt_cluster = state_yt_cluster
        self.state_locke_path = state_locke_path
        self.state_locke_token = state_locke_token
        self.client = None
        self.transaction = None
        self.stage_transaction = None

    def __enter__(self):
        self.client = yt.YtClient(proxy=self.state_yt_cluster, token=self.state_locke_token)
        self.transaction = yt.Transaction(timeout=600 * 1000, client=self.client)
        self.transaction.__enter__()
        try:
            if self._save:
                if not self.client.exists(self.state_locke_path):
                    self.client.create(type="document", path=self.state_locke_path)
                    self.client.set(self.state_locke_path, self.default_data())
                logger.info("Acquiring lock for path: \"{0}\"".format(self.state_locke_path))
                self.client.lock(self.state_locke_path, waitable=True, wait_for=600 * 1000)
                self._data = self.client.get(self.state_locke_path)
                self.stage_transaction = yt.Transaction(timeout=600 * 1000, client=self.client)
                self.stage_transaction.__enter__()
            else:
                if self.client.exists(self.state_locke_path):
                    self._data = self.client.get(self.state_locke_path)
                else:
                    self._data = self.default_data()
        except Exception as err:
            logger.info("Error during state loading, state will not be modified!")
            self.transaction.__exit__(type(err), err, None)
            raise err

    def __exit__(self, error_type, error_value, error_traceback):
        if error_value:
            logger.info("Error during resources processing, state will not be modified for this objects group")
        if self.stage_transaction:
            self.stage_transaction.__exit__(error_type, error_value, error_traceback)
        self.transaction.__exit__(None, None, None)

    def save(self):
        if not self._save:
            return
        logger.debug("Saving state to locke node at \"{0}\"".format(self.state_locke_path))
        self.client.set(self.state_locke_path, self._data)
        self.stage_transaction.__exit__(None, None, None)
        self.stage_transaction = yt.Transaction(timeout=600 * 1000, client=self.client)
        self.stage_transaction.__enter__()


class NoneState(State):
    def __init__(self, save, delete_untracked):
        super(NoneState, self).__init__(save, delete_untracked)

    def __enter__(self):
        self._data = self.default_data()

    def __exit__(self, error_type, error_value, error_traceback):
        pass

    def save(self):
        pass


def init_state(save, delete_untracked, configuration):
    state = None

    def check_not_initialized():
        if state is not None:
            raise Exception("State is already initialized: {0}".format(state))

    if configuration.get("state_file_path"):
        check_not_initialized()
        state = LocalFileState(save, delete_untracked, configuration["state_file_path"])
    if configuration.get("state_locke_path") and configuration.get("state_locke_token"):
        check_not_initialized()
        state = LockeNodeState(save, delete_untracked, configuration.get("state_yt_cluster", "locke"), configuration["state_locke_path"], configuration["state_locke_token"])
    if not state:
        state = NoneState(save, delete_untracked)
    logger.info("Using {0} state class!".format(state.__class__.__name__))
    return state
