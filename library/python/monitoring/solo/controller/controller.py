# -*- coding: utf-8 -*-
import itertools
import logging

import click
import six
from six.moves import filter

if six.PY3:
    import asyncio

from library.python.monitoring.solo.controller.state import init_state
from library.python.monitoring.solo.controller.resource import ResourceAction, Resource, MODIFYING_ACTIONS
from library.python.monitoring.solo.handlers.solomon import group_by_solomon_handlers
from library.python.monitoring.solo.handlers.juggler import group_by_juggler_handlers
from library.python.monitoring.solo.handlers.yasm import group_by_yasm_handlers
from library.python.monitoring.solo.util.diff import print_diff
from library.python.monitoring.solo.objects.solomon.v2 import SolomonObject
from library.python.monitoring.solo.objects.solomon.v3 import Message
from library.python.monitoring.solo.objects.juggler import Check, JugglerObject
from library.python.monitoring.solo.objects.yasm import YasmObject, YasmPanel

logger = logging.getLogger(__name__)


class Controller(object):

    def __init__(
        self,
        save=False,
        delete_untracked=False,
        clear_untracked_in_state=False,
        **configuration
    ):
        """
        Constructs Controller object
        :param save:
            False - print diff on "process" call
            True - print diff and execute actions on "process" call
        :type save: bool
        :param delete_untracked:
            False - do not delete untracked resources, do not clear state
            True - delete untracked resources, clear state
        :type delete_untracked: bool

        # State:
        ## LocalFileState:
        :param state_file_path: path to the file with state
        :type state_file_path: str
        ## LockeNodeState:
        :param state_yt_cluster: name of YT cluster (proxy). Default: locke
        :type state_yt_cluster: str
        :param state_locke_path: path to the locke node with state
        :type state_locke_path: str
        :param state_locke_token: OAuth token for Locke
        :type state_locke_token: str
        ## NoneState:

        # Solomon:
        :param solomon_token: OAuth token for Solomon
        :type solomon_token: str

        # Juggler:
        :param juggler_token: OAuth token for Juggler
        :type juggler_token: str
        :param juggler_mark: Identifier used for tagging Juggler checks
        :type juggler_mark: str
        """
        self.save = save
        self.delete_untracked = delete_untracked
        self.state = init_state(save, delete_untracked, configuration)
        self.configuration = configuration

    def _convert_to_resources(self, objects_registry):
        resources = []
        for obj in objects_registry:
            if isinstance(obj, SolomonObject):
                resources.append(Resource(
                    local_id="{0}.{1}.{2}".format(obj.__class__.__OBJECT_TYPE__.lower(), obj.project_id, obj.id),
                    local_state=obj
                ))
            elif isinstance(obj, Message):
                resources.append(Resource(
                    local_id="{0}.{1}.{2}".format(obj.DESCRIPTOR.name.lower(), obj.project_id, obj.id),
                    local_state=obj
                ))
            elif isinstance(obj, Check):
                resources.append(Resource(
                    local_id="{0}.{1}".format(obj.host, obj.service),
                    local_state=obj
                ))
            elif isinstance(obj, JugglerObject):
                resources.append(Resource(
                    local_id="{0}.{1}".format(obj.__class__.__OBJECT_TYPE__.lower(), obj.address),
                    local_state=obj
                ))
            elif isinstance(obj, YasmObject):
                if isinstance(obj, YasmPanel):
                    local_id = "yasm.{0}.{1}.{2}".format(obj.__class__.__OBJECT_TYPE__.lower(), obj.user, obj.id)
                else:
                    local_id = "yasm.{0}.{1}".format(obj.__class__.__OBJECT_TYPE__.lower(), obj.id)
                resources.append(Resource(local_id=local_id, local_state=obj))
            else:
                raise Exception("Can't convert to resource resource: {0}".format(obj))
        return resources

    def _group_resources_by_handlers(self, resources):
        try:
            return itertools.chain(
                group_by_solomon_handlers(self.configuration, resources),
                group_by_juggler_handlers(self.configuration, resources),
                group_by_yasm_handlers(resources)
            )
        except KeyError as err:
            raise Exception("Can't construct handler: {0}".format(err))

    def _sync_resources(self, handler, handler_resources):
        logger.info("Syncing resources state!")
        if six.PY3:
            asyncio.get_event_loop().run_until_complete(
                asyncio.gather(*[handler.get(resource) for resource in handler_resources])
            )
        else:
            for resource in handler_resources:
                handler.get(resource)

    def _calculate_actions(self, handler, handler_resources):
        logger.info("Calculating actions!")
        for resource in handler_resources:
            diff_header = "diff for: {0}".format(resource.local_id)
            if resource.provider_state is None:
                if resource.local_state is None:
                    # obsolete untracked resource in state - fix state
                    resource.action = ResourceAction.IGNORE
                else:
                    print_diff(diff_header, handler.diff(resource))
                    resource.action = ResourceAction.CREATE
            else:
                if resource.local_state is None:
                    print_diff(diff_header, handler.diff(resource))
                    resource.action = ResourceAction.DELETE
                else:
                    diff = handler.diff(resource)
                    if diff:
                        print_diff(diff_header, diff)
                        resource.action = ResourceAction.MODIFY
                    else:
                        resource.action = ResourceAction.SKIP
        for resource_action in [ResourceAction.CREATE, ResourceAction.MODIFY, ResourceAction.DELETE, ResourceAction.SKIP, ResourceAction.IGNORE]:
            filtered_resources = list(filter(lambda resource: resource.action == resource_action, handler_resources))
            if filtered_resources:
                method = 'debug' if resource_action == ResourceAction.SKIP else 'info'
                getattr(logger, method)(
                    click.style(
                        "Action {0}: {1} resources: {2}".format(resource_action, len(filtered_resources), [r.local_id for r in filtered_resources]),
                        fg="white" if resource_action == ResourceAction.SKIP else "yellow"
                    )
                )

    def _execute_actions(self, handler, handler_resources):
        logger.info("Executing actions!")
        if six.PY3:
            actions = [
                getattr(handler, resource.action.lower())(resource) for resource in filter(lambda r: r.action in MODIFYING_ACTIONS, handler_resources)
            ]
            if actions:
                asyncio.get_event_loop().run_until_complete(asyncio.gather(*actions))
        else:
            for resource in filter(lambda r: r.action in MODIFYING_ACTIONS, handler_resources):
                getattr(handler, resource.action.lower())(resource)

    def process(self, objects_registry):
        resources = self._convert_to_resources(objects_registry)

        handler_list = []
        try:
            with self.state:
                # fill provider_id and retrieve untracked resources
                self.state.get_resources_provider_id(resources)
                resources += self.state.get_untracked_resources(resources)

                # group resources by handlers and execute required actions
                for handler, handler_info, handler_resources in self._group_resources_by_handlers(resources):
                    if handler not in handler_list:
                        handler_list.append(handler)
                    logger.info("======================================")
                    logger.info("Processing {0}".format(handler_info))
                    self._sync_resources(handler, handler_resources)
                    self._calculate_actions(handler, handler_resources)
                    if self.save:
                        self._execute_actions(handler, handler_resources)
                    self.state.set_resources_provider_id(handler_resources)
        finally:
            for handler in handler_list:
                handler.finish()
