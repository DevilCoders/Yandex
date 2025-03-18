import requests
import logging
import sys
import json
from datetime import datetime  # noqa

import six.moves.urllib as urllib
from retry.api import retry_call  # PyPI support

from . import reactor_objects as r_objs  # noqa
from . import reactor_schemas as r_schemas
from . import client_base

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stderr)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


class RetryPolicy(object):
    def __init__(self, tries=4, delay=0, max_delay=None, backoff=1, jitter=0):
        """
        :param tries: the maximum number of attempts. default: 4, -1 means infinite.
        :param delay: initial delay between attempts. default: 0.
        :param max_delay: the maximum value of delay. default: None (no limit).
        :param backoff: multiplier applied to delay between attempts. default: 1 (no backoff).
        :param jitter: extra seconds added to delay between attempts. default: 0.
                       fixed if a number, random if a range tuple (min, max)
        """
        self.tries = tries
        self.delay = delay
        self.max_delay = max_delay
        self.backoff = backoff
        self.jitter = jitter


def make_request_info_str(request_json, request_url):
    """
    :type request_json: str
    :type request_url: str
    :retype: str
    """

    message = "Request url {}.\nRequest content:\n".format(request_url)
    message += str(request_json) + "\n"
    return message


class ReactorRequester(object):
    DEFAULT_TIMEOUT = 30
    RETRIED_EXCEPTIONS = (client_base.ReactorAPITimeout, client_base.ReactorInternalError, requests.ConnectionError)

    def __init__(self, api_url, token, retry_policy=None, timeout=DEFAULT_TIMEOUT, handle_ts_zone=False):
        """
        :type api_url: str
        :type token: str
        :param retry_policy: No retries if None
        :type retry_policy: RetryPolicy | None
        :type timeout: int
        :type handle_ts_zone: bool
        """
        self._token = token
        self._timeout = timeout
        self._api_url = api_url
        self._check_url(self._api_url)

        self._retry_policy = retry_policy

        self._headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": "OAuth {}".format(self._token)
        }
        if handle_ts_zone:
            self._headers['X-Reactor-Enable-Iso-Timestamps'] = 'True'

    def send_request(
        self,
        end_point,
        json=None,
        params=None,
        timeout=None,
        method="post",
        cookies=None,
        force_no_retries=False,
    ):
        timeout = timeout if timeout is not None else self._timeout
        if self._retry_policy is None or force_no_retries:
            return self._send_request(end_point, json, params, timeout, method, cookies)
        else:
            return retry_call(
                self._send_request,
                fkwargs=dict(
                    end_point=end_point,
                    json=json,
                    params=params,
                    timeout=timeout,
                    method=method,
                    cookies=cookies,
                ),
                exceptions=self.RETRIED_EXCEPTIONS,
                tries=self._retry_policy.tries,
                delay=self._retry_policy.delay,
                max_delay=self._retry_policy.max_delay,
                backoff=self._retry_policy.backoff,
                jitter=self._retry_policy.jitter
            )

    def _send_request(self, end_point, json=None, params=None, timeout=DEFAULT_TIMEOUT, method="post", cookies=None):
        url = self._api_url + "/" + end_point

        request_info_str = make_request_info_str(request_url=url, request_json=json)

        try:
            response = requests.request(method=method, url=url, json=json,
                                        params=params, timeout=timeout,
                                        headers=self._headers, cookies=cookies)
            if not response.ok:
                raise self.parse_error(request_info_str, response)

        except requests.Timeout:
            message = "Timeout {} occurs when connecting to {} url".format(timeout, url)
            raise client_base.ReactorAPITimeout(504, message)

        except client_base.ReactorAPIException:
            raise

        except Exception:
            logging.exception(request_info_str)
            raise

        return response

    @staticmethod
    def parse_error(request_info_str, response):
        message = "Response contains error with code: {}\nReason: {}\nResponse:\n"
        response_text = response.text
        error_json = None
        try:
            error_json = response.json()
            response_text = json.dumps(error_json, indent=4, sort_keys=True)
        except Exception as e:
            logger.exception(e)

        # append formatted json (or raw response when json cannot be parsed), see DMP-87
        message += response_text + "\n" + request_info_str

        if error_json is None or response.status_code == 503:
            if error_json is None:
                logger.error(message)
            raise client_base.ReactorInternalError(response.status_code, message)

        # since this moment, error_json contains valid parsed response
        if "code" in error_json:
            codes = {error_json["code"]}
        elif "codes" in error_json:
            codes = {err["code"] for err in error_json["codes"]}
        else:
            codes = set()

        if "NAMESPACE_DOES_NOT_EXIST" in codes:
            return client_base.NoNamespaceError(response.status_code, message)
        elif "ARTIFACT_DOES_NOT_EXIST_ERROR" in codes:
            return client_base.NoArtifactError(response.status_code, message)
        elif "OPERATION_DOES_NOT_EXIST_ERROR" in codes:
            return client_base.NoReactionError(response.status_code, message)

        return client_base.ReactorInternalError(response.status_code, message)

    @property
    def uses_retries(self):
        """
        :rtype: bool
        """
        return self._retry_policy is not None

    @staticmethod
    def _check_url(url):
        parsed_url = urllib.parse.urlparse(url)
        if parsed_url.scheme not in ("http", "https"):
            raise client_base.ReactorAPIException(
                message="API url must be prefixed with `http` or `https`",
                status_code=500)


class ClientEndpoint(object):
    def __init__(self, reactor_requester):
        """
        :type reactor_requester: ReactorRequester
        """
        self._requester = reactor_requester

    @property
    def requester(self):
        """
        :rtype: ReactorRequester
        """
        return self._requester


class ArtifactTypeEndpoint(ClientEndpoint, client_base.ArtifactTypeEndpointBase):
    def get(self, artifact_type_id=None, artifact_type_key=None):
        """
        :type artifact_type_id: int
        :type artifact_type_key: str
        :rtype: r_objs.ArtifactType
        """
        data = r_schemas.to_json(r_objs.ArtifactTypeIdentifier(artifact_type_id, artifact_type_key),
                                 r_schemas.ArtifactTypeIdentifierSchema())
        response = self.requester.send_request("a/t/get", json={"artifactTypeIdentifier": data}, method="post")
        return r_schemas.from_json(response.json()["artifactType"], r_schemas.ArtifactTypeSchema())

    def list(self):
        """
        :rtype: list[r_objs.ArtifactType]
        """
        response = self.requester.send_request(
            "a/t/list",
            method="post",
            json={}
        ).json()

        return [
            r_schemas.from_json(r, r_schemas.ArtifactTypeSchema())
            for r in response["artifactTypes"]
        ]


class DynamicTriggerEndpoint(ClientEndpoint, client_base.DynamicTriggerEndpointBase):
    def add(self, reaction_identifier, triggers):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type triggers: r_objs.DynamicTriggerList
        :return: list[r_objs.DynamicTriggerDescriptor]
        """
        reaction_identifier = r_schemas.to_json(
            reaction_identifier, r_schemas.OperationIdentifierSchema()
        )
        triggers = r_schemas.to_json(
            triggers, r_schemas.DynamicTriggerListSchema()
        )
        response = self.requester.send_request(
            "t/add",
            json={
                'reactionIdentifier': reaction_identifier,
                'triggers': triggers
            },
            method="post"
        )
        return [
            r_schemas.from_json(
                trigger, r_schemas.DynamicTriggerDescriptorSchema()
            ) for trigger in response.json()['triggers']
        ]

    def remove(self, reaction_identifier, triggers):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type triggers: List[r_objs.DynamicTriggerIdentifier]
        :return: {}
        """
        reaction_identifier = r_schemas.to_json(
            reaction_identifier, r_schemas.OperationIdentifierSchema()
        )
        triggers = [
            r_schemas.to_json(
                trigger, r_schemas.DynamicTriggerIdentifierSchema()
            ) for trigger in triggers
        ]

        response = self.requester.send_request(
            "t/remove",
            json={
                'reactionIdentifier': reaction_identifier,
                'triggerIdentifier': triggers
            },
            method="post"
        )
        return response.json()

    def update(self, reaction_identifier, triggers, action):
        """
       :type reaction_identifier: r_objs.OperationIdentifier
       :type triggers: List[r_objs.DynamicTriggerIdentifier]
       :type action: r_objs.DynamicTriggerAction
       :return: {}
       """
        data = r_schemas.to_json(
            r_objs.DynamicTriggerStatusUpdate(
                reaction_identifier, triggers, action
            ),
            r_schemas.DynamicTriggerStatusUpdateSchema()
        )

        response = self.requester.send_request(
            't/update',
            json=data,
            method='post'
        )
        return response.json()

    def list(self, reaction_identifier):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :return: list[r_objs.DynamicTriggerDescriptor]
        """
        data = r_schemas.to_json(
            reaction_identifier,
            r_schemas.OperationIdentifierSchema()
        )
        response = self.requester.send_request(
            't/list',
            json={'reactionIdentifier': data},
            method='post'
        ).json()

        return [
            r_schemas.from_json(
                trigger, r_schemas.DynamicTriggerDescriptorSchema()
            ) for trigger in response['triggers']
        ]


class ArtifactTriggerEndpoint(ClientEndpoint, client_base.ArtifactTriggerEndpointBase):

    def insert(self, reaction_identifier, artifact_instance_ids):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type artifact_instance_ids: List[int]
        :return: {}
        """
        data = {
            "reactionIdentifier": r_schemas.to_json(reaction_identifier, r_schemas.OperationIdentifierSchema()),
            "artifactInstanceIds": [str(a_id) for a_id in artifact_instance_ids]
        }

        response = self.requester.send_request(
            "a/t/insert",
            json=data,
            method="post"
        )

        return response.json()


class ArtifactEndpoint(ClientEndpoint, client_base.ArtifactEndpointBase):
    def get(self, artifact_id=None, namespace_identifier=None):
        """
        Use either `artifact_id` or `namespace_identifier`
        :type artifact_id: int
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :raises: ArtifactNotExists
        :rtype: r_objs.Artifact
        """
        data = r_schemas.to_json(r_objs.ArtifactIdentifier(artifact_id, namespace_identifier),
                                 r_schemas.ArtifactIdentifierSchema())
        response = self.requester.send_request("a/get", json={"artifactIdentifier": data}, method="post")
        return r_schemas.from_json(response.json()["artifact"], r_schemas.ArtifactSchema())

    def create(self, artifact_type_identifier, artifact_identifier,
               description, permissions,
               project_identifier=None, cleanup_strategy=None,
               create_parent_namespaces=False, create_if_not_exist=False):
        """
        :type artifact_type_identifier: r_objs.ArtifactTypeIdentifier
        :type artifact_identifier: r_objs.ArtifactIdentifier
        :type description: str
        :type permissions: r_objs.NamespacePermissions
        :type create_parent_namespaces: bool
        :type create_if_not_exist: bool
        :type project_identifier: ProjectIdentifier
        :type cleanup_strategy: CleanupStrategyDescriptor
        :rtype: r_objs.ArtifactCreateResponse
        """
        request_object = r_objs.ArtifactCreateRequest(
            artifact_type_identifier=artifact_type_identifier,
            artifact_identifier=artifact_identifier,
            description=description,
            permissions=permissions,
            create_parent_namespaces=create_parent_namespaces,
            create_if_not_exist=create_if_not_exist,
            project_identifier=project_identifier,
            cleanup_strategy=cleanup_strategy
        )

        data = r_schemas.to_json(request_object,
                                 r_schemas.ArtifactCreateRequestSchema())
        response = self.requester.send_request("a/create", json=data, method="post")
        return r_schemas.from_json(response.json(), r_schemas.ArtifactCreateResponseSchema())

    def check_exists(self, artifact_id=None, namespace_identifier=None):
        """
        Use either `artifact_id` or `namespace_identifier`
        :type artifact_id: int
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: bool
        """
        try:
            self.get(artifact_id, namespace_identifier)
        except client_base.NoArtifactError:
            return False
        else:
            return True


class ArtifactInstanceEndpoint(ClientEndpoint, client_base.ArtifactInstanceEndpointBase):

    def instantiate(
        self,
        artifact_identifier=None,
        metadata=None,
        attributes=None,
        user_time=None,
        builder=None,
        create_if_not_exist=False,
    ):
        """
        :type artifact_identifier: ArtifactIdentifier
        :type metadata: Metadata
        :type attributes: Attributes
        :type user_time: datetime
        :type builder: helper.artifact_instance.ArtifactInstanceBuilder
        :type create_if_not_exist: bool
        :rtype: r_objs.ArtifactInstanceInstantiateResponse
        """
        force_no_retries = False
        if self.requester.uses_retries and not create_if_not_exist:
            # Retries can accidentally publish instance twice triggering all dependent reactions twice
            # Setting create_id_not_exist=True prevents double creation of same artifact and enables retries
            # TODO: test this behavior
            force_no_retries = True

        if builder:
            data = r_schemas.to_json(r_objs.ArtifactInstanceInstantiateRequest(
                artifact_identifier=builder.artifact_identifier,
                metadata=builder.value,
                attributes=builder.attributes,
                user_time=builder.user_time,
                create_if_not_exist=create_if_not_exist),
                r_schemas.ArtifactInstanceInstantiateRequestSchema())
        else:
            data = r_schemas.to_json(r_objs.ArtifactInstanceInstantiateRequest(
                artifact_identifier=artifact_identifier,
                metadata=metadata,
                attributes=attributes,
                user_time=user_time,
                create_if_not_exist=create_if_not_exist),
                r_schemas.ArtifactInstanceInstantiateRequestSchema())
        response = self.requester.send_request(
            "a/i/instantiate",
            json=data,
            method="post",
            force_no_retries=force_no_retries,
        )
        return r_schemas.from_json(response.json(), r_schemas.ArtifactInstanceInstantiateResponseSchema())

    def last(self, artifact_identifier):
        """
        :type artifact_identifier: r_objs.ArtifactIdentifier
        :rtype: r_objs.ArtifactInstance
        """
        data = r_schemas.to_json(artifact_identifier, r_schemas.ArtifactIdentifierSchema())
        response = self.requester.send_request("a/i/get/last", json={"artifactIdentifier": data}, method="post")
        if "result" in response.json():
            return r_schemas.from_json(response.json()["result"], r_schemas.ArtifactInstanceSchema())
        return None

    def range(self, filter_):
        """
        :type filter_: r_objs.ArtifactInstanceFilterDescriptor
        :rtype: list[r_objs.ArtifactInstance]
        """
        data = r_schemas.to_json(filter_, r_schemas.ArtifactInstanceFilterDescriptorSchema())
        response = self.requester.send_request("a/i/get/range", json={"filter": data}, method="post")
        return [r_schemas.from_json(obj, r_schemas.ArtifactInstanceSchema()) for obj in response.json()["range"]]

    def get_status_history(self, artifact_instance_id):
        """
        :type artifact_instance_id: int
        :rtype: list[r_objs.ArtifactInstanceStatusRecord]
        """

        data = {
            "artifactInstanceId": str(artifact_instance_id)
        }
        response = self.requester.send_request("a/i/getStatusHistory", json=data, method="post")
        return [
            r_schemas.from_json(obj, r_schemas.ArtifactInstanceStatusRecordSchema())
            for obj in response.json()["artifactInstanceStatuses"]
        ]

    def deprecate(self, instances_to_deprecate, description):
        """
        :type instances_to_deprecate: List[int]
        :type description: str
        :rtype: Optional[int]
        """
        request = r_objs.ArtifactInstancesDeprecationRequest(instances_to_deprecate, description)
        data = r_schemas.to_json(request, r_schemas.ArtifactInstancesDeprecationRequestSchema())
        response = self.requester.send_request("a/i/deprecate", json=data, method="post")

        replacement_instance = None
        if "artifactInstanced" in response.json():
            replacement_instance = int(response.json()["artifactInstanced"])

        deprecated = None
        if "deprecatedInstancesIds" in response.json():
            deprecated = [int(v) for v in response.json()["deprecatedInstancesIds"]]

        return replacement_instance, deprecated


class ReactionEndpoint(ClientEndpoint, client_base.ReactionEndpointBase):

    def create(
        self,
        operation_descriptor=None,
        create_if_not_exist=False,
        replacement_kind=None,
        old_version_for_replacement=None,
        replacement_description=None,
    ):
        """
        :type operation_descriptor: r_objs.OperationDescriptor
        :type create_if_not_exist: bool
        :type replacement_kind: r_objs.ReactionReplacementKind
        :type old_version_for_replacement: r_objs.OperationIdentifier
        :type replacement_description: str
        :rtype: r_objs.ReactionReference
        """

        if self.requester.uses_retries and not create_if_not_exist:
            # Retries can accidentally publish instance twice triggering all dependent reactions twice
            # Setting create_id_not_exist=True prevents double creation of same artifact and enables retries
            raise client_base.ReactorAPIException(status_code=500,
                                                  message="Do not use automatic retries with create_if_not_exist=False")

        data = r_schemas.to_json(operation_descriptor,
                                 r_schemas.OperationDescriptorSchema())

        if replacement_kind is not None:
            json = {
                "reaction": data,
                "createIfNotExist": create_if_not_exist,
                "replacementKind": replacement_kind.name,
                "oldVersionForReplacement": r_schemas.to_json(
                    old_version_for_replacement, r_schemas.OperationIdentifierSchema()
                ),
                "replacementDescription": replacement_description,
            }
        else:
            json = {"reaction": data, "createIfNotExist": create_if_not_exist}

        response = self.requester.send_request("r/create", json=json, method="post")
        return r_schemas.from_json(response.json(), r_schemas.ReactionReferenceSchema())

    def get(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :raises: ReactionNotExists
        :rtype: r_objs.Operation
        """
        data = r_schemas.to_json(operation_identifier,
                                 r_schemas.OperationIdentifierSchema())

        response = self.requester.send_request(
            "r/get",
            json={"reaction": data},
            method="post"
        )

        return r_schemas.from_json(response.json()["reaction"], r_schemas.OperationSchema())

    # TODO: merge .get_queue() with .get()
    def get_queue(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :raises: ReactionNotExists
        :rtype: r_objs.Queue
        """
        data = r_schemas.to_json(operation_identifier,
                                 r_schemas.OperationIdentifierSchema())

        response = self.requester.send_request(
            "r/get",
            json={"reaction": data},
            method="post"
        )

        response_json = response.json()

        if "queue" in response_json:
            return r_schemas.from_json(response_json["queue"], r_schemas.QueueSchema())
        else:
            return None

    def check_exists(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :rtype: bool
        """
        try:
            self.get(operation_identifier)
        except client_base.NoReactionError:
            return False
        else:
            return True

    def update(self, status_update_list=None, reaction_start_configuration_update=None):
        """
        :type status_update_list: list[r_objs.ReactionStatusUpdate]
        :type reaction_start_configuration_update: list[r_objs.ReactionStartConfigurationUpdate]
        :return: {}
        """
        json_dict = {}
        status_update_data = [
            r_schemas.to_json(o, r_schemas.ReactionStatusUpdateSchema()) for o in status_update_list
        ] if status_update_list is not None else None
        if status_update_data is not None:
            json_dict["statusUpdates"] = status_update_data

        start_configuration_update_data = [
            r_schemas.to_json(o, r_schemas.ReactionStartConfigurationUpdateSchema())
            for o in reaction_start_configuration_update
        ] if reaction_start_configuration_update is not None else None
        if start_configuration_update_data is not None:
            json_dict["startConfigurationUpdate"] = start_configuration_update_data

        response = self.requester.send_request(
            "r/update",
            json=json_dict,
            method="post"
        )

        return response.json()


class ReactionInstanceEndpoint(ClientEndpoint, client_base.ReactionInstanceEndpointBase):
    def _list(self, reaction, bound_instance_id, filter_type, order, limit):
        obj = r_objs.ReactionInstanceListRequest(
            reaction=reaction,
            bound_instance_id=bound_instance_id,
            filter_type=filter_type,
            order=order,
            limit=limit
        )
        data = r_schemas.to_json(obj, r_schemas.ReactionInstanceListRequestSchema())
        response = self.requester.send_request("r/i/list", json=data, method="post")
        return response.json()["reactionInstances"]

    def list(self, reaction, bound_instance_id=None, filter_type=r_objs.FilterTypes.GREATER_THAN,
             order=r_objs.OrderTypes.DESCENDING, limit=100):
        """
        :type reaction: r_objs.OperationIdentifier
        :type bound_instance_id: int | None
        :type filter_type: r_objs.FilterTypes
        :type order: r_objs.OrderTypes
        :param limit: limit lies between 1 and 100
        :type limit: int
        :rtype: list[r_objs.OperationInstance]
        """
        resp_json_array = self._list(reaction, bound_instance_id, filter_type, order, limit)
        return [r_schemas.from_json(d, r_schemas.OperationInstanceSchema()) for d in resp_json_array]

    def list_statuses(self, reaction, bound_instance_id=None, filter_type=r_objs.FilterTypes.GREATER_THAN,
                      order=r_objs.OrderTypes.DESCENDING, limit=100):
        """
        :type reaction: r_objs.OperationIdentifier
        :type bound_instance_id: int | None
        :type filter_type: r_objs.FilterTypes
        :type order: r_objs.OrderTypes
        :type limit: int
        :rtype: list[r_objs.OperationInstanceStatusView]
        """
        resp_json_array = self._list(reaction, bound_instance_id, filter_type, order, limit)
        return [r_schemas.from_json(d, r_schemas.OperationInstanceStatusViewSchema()) for d in resp_json_array]

    def cancel(self, reaction_instance_id):
        """
        :type reaction_instance_id: int
        """
        self.requester.send_request(
            "r/i/cancel",
            json={"reactionInstanceId": str(reaction_instance_id)},
            method="post"
        )

    def get(self, reaction_instance_id):
        """
        :type reaction_instance_id: int
        :rtype: r_objs.OperationInstance
        """
        response = self.requester.send_request(
            "r/i/get",
            json={"reactionInstanceId": str(reaction_instance_id)},
            method="post"
        )

        return r_schemas.from_json(response.json()["reactionInstance"], r_schemas.OperationInstanceSchema())

    def get_status_history(self, reaction_instance_id):
        """
        :param reaction_instance_id: int
        :rtype: list[r_objs.OperationInstanceStatus]
        """
        response = self.requester.send_request(
            "r/i/getStatusHistory",
            json={"reactionInstanceId": str(reaction_instance_id)},
            method="post"
        )
        return [
            r_schemas.from_json(d, r_schemas.OperationInstanceStatusSchema())
            for d in response.json()["reactionInstanceStatuses"]
        ]


class NamespaceEndpoint(ClientEndpoint, client_base.NamespaceEndpointBase):
    def get(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :raise: NamespaceNotExists
        :rtype: r_objs.Namespace
        """
        data = r_schemas.to_json(namespace_identifier, r_schemas.NamespaceIdentifierSchema())
        response = self.requester.send_request(
            "n/get",
            json={"namespaceIdentifier": data},
            method="post"
        )
        return r_schemas.from_json(response.json()["namespace"], r_schemas.NamespaceSchema())

    def create(self, namespace_identifier, description, permissions, create_parents=False, create_if_not_exist=True):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :type description: str
        :type  permissions: r_objs.NamespacePermissions
        :type create_parents: bool
        :type create_if_not_exist: bool
        :rtype: int
        """
        data = r_schemas.to_json(r_objs.NamespaceCreateRequest(
            namespace_identifier=namespace_identifier,
            description=description,
            permissions=permissions,
            create_parents=create_parents,
            create_if_not_exist=create_if_not_exist),
            r_schemas.NamespaceCreateRequestSchema())
        response = self.requester.send_request("n/create", json=data, method="post")
        return r_schemas.from_json(response.json(), r_schemas.NamespaceCreateResponseSchema())

    def delete(self, namespace_identifier_list, delete_if_exist=False, force_delete=False):
        """
        :type namespace_identifier_list: list[r_objs.NamespaceIdentifier]
        :type delete_if_exist: Bool
        :type force_delete: Bool
        :return: {}
        """
        data = {
            "namespaceIdentifier": [
                r_schemas.to_json(i, r_schemas.NamespaceIdentifierSchema()) for i in namespace_identifier_list
            ],
            "deleteIfExist": delete_if_exist,
            "forceDelete": force_delete
        }

        response = self.requester.send_request("n/delete", json=data, method="post")
        return response.json()

    def check_exists(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: bool
        """
        try:
            self.get(namespace_identifier)
        except client_base.NoNamespaceError:
            return False
        else:
            return True

    def list(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[r_objs.Namespace]
        """

        data = r_schemas.to_json(namespace_identifier, r_schemas.NamespaceIdentifierSchema())
        response = self.requester.send_request(
            "n/list",
            json={"namespaceIdentifier": data},
            method="post"
        )

        return [r_schemas.from_json(n, r_schemas.NamespaceSchema()) for n in response.json()["namespaces"]]

    def list_names(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[str]
        """
        data = r_schemas.to_json(namespace_identifier, r_schemas.NamespaceIdentifierSchema())
        response = self.requester.send_request(
            "n/list-names",
            json={"namespaceIdentifier": data},
            method="post"
        )
        return response.json()["names"]

    def resolve(self, entity_id):
        """
        :type entity_id: int
        :rtype: str
        """
        data = r_schemas.to_json(
            r_objs.EntityReference(type_="PROJECT", entity_id=str(entity_id)), r_schemas.EntityReferenceSchema()
        )
        response = self.requester.send_request(
            "n/resolve",
            json={"entityReference": data, "resolvePath": True},
            method="post"
        )
        return response.json()["path"]

    def resolve_path(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: str
        """
        data = r_schemas.to_json(namespace_identifier, r_schemas.NamespaceIdentifierSchema())
        response = self.requester.send_request(
            "n/resolve/path",
            json={"namespaceIdentifier": data},
            method="post"
        )
        return response.json()["path"]

    def move(self, source_identifier, destination_identifier):
        """
        :type source_identifier: r_objs.NamespaceIdentifier
        :type destination_identifier: r_objs.NamespaceIdentifier
        :return: {}
        """
        data = {
            "sourceIdentifier": r_schemas.to_json(source_identifier, r_schemas.NamespaceIdentifierSchema()),
            "destinationIdentifier": r_schemas.to_json(destination_identifier, r_schemas.NamespaceIdentifierSchema()),
        }
        response = self.requester.send_request(
            "n/move",
            json=data,
            method="post"
        )
        return response.json()


class ReactionTypeEndpoint(ClientEndpoint):
    def list(self):
        """
        Ignores parameter descriptors
        :rtype: list[r_objs.ReactionType]
        """
        response = self.requester.send_request(
            "r/t/list",
            json={},
            method="post"
        ).json()

        # Typo in API. Will be fixed in next release but we support it now
        key = "operatioTypes" if "operatioTypes" in response else "reactionTypes"

        return [
            r_schemas.from_json(r, r_schemas.ReactionTypeSchema())
            for r in response[key]
        ]


class NamespaceNotificationEndpoint(ClientEndpoint, client_base.NamespaceNotificationEndpointBase):
    def change_long_running(self, operation_identifier, options):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :type options: r_objs.LongRunningOperationInstanceNotificationOptions
        :return: {}
        """
        return self.requester.send_request(
            "r/looong-running/change",
            json={
                "reaction": r_schemas.to_json(operation_identifier, r_schemas.OperationIdentifierSchema()),
                "newOptions": r_schemas.to_json(
                    options, r_schemas.LongRunningOperationInstanceNotificationOptionsSchema()
                ),
            },
            method="post"
        ).json()

    def get_long_running(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :rtype: r_objs.LongRunningOperationInstanceNotificationOptions | None
        """
        data = self.requester.send_request(
            "r/looong-running/get",
            json={
                "reaction": r_schemas.to_json(operation_identifier, r_schemas.OperationIdentifierSchema())
            },
            method="post"
        ).json()

        return r_schemas.from_json(
            data["options"], r_schemas.LongRunningOperationInstanceNotificationOptionsSchema()
        ) if data else None

    def change(self, delete_id_list=None, notification_descriptor_list=None):
        """
        :type delete_id_list: list[int] | None
        :type notification_descriptor_list: list[r_objs.NotificationDescriptor] | None
        """

        new_notification_dumps = [
            r_schemas.to_json(n, r_schemas.NotificationDescriptorSchema()) for n in notification_descriptor_list
        ] if notification_descriptor_list is not None else []

        request = {
            "notificationIdsToDelete": [str(i) for i in delete_id_list] if delete_id_list is not None else [],
            "notificationsToCreate": new_notification_dumps
        }

        return self.requester.send_request(
            "n/notification/change",
            json=request,
            method="post"
        ).json()

    def list(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[r_objs.Notification]
        """

        data = r_schemas.to_json(namespace_identifier, r_schemas.NamespaceIdentifierSchema())
        response = self.requester.send_request(
            "n/notification/list",
            json={"namespace": data},
            method="post"
        )
        return [r_schemas.from_json(n, r_schemas.NotificationSchema()) for n in response.json()["notifications"]]


class QueueEndpoint(ClientEndpoint, client_base.QueueEndpointBase):
    def create(self, queue_descriptor, create_if_not_exist=False):
        """
        :type queue_descriptor: r_objs.QueueDescriptor
        :type create_if_not_exist: bool
        :rtype: r_objs.QueueReference
        """
        data = r_schemas.to_json(queue_descriptor, r_schemas.QueueDescriptorSchema())

        response = self.requester.send_request(
            "q/create",
            json={"queueDescriptor": data, "createIfNotExist": create_if_not_exist},
            method="post"
        )

        return r_schemas.from_json(response.json(), r_schemas.QueueReferenceSchema())

    def get(self, queue_identifier):
        """
        :type queue_identifier: r_objs.QueueIdentifier
        :rtype: r_objs.QueueReactions
        """
        data = r_schemas.to_json(queue_identifier, r_schemas.QueueIdentifierSchema())

        response = self.requester.send_request(
            "q/get",
            json={"queueIdentifier": data},
            method="post"
        )

        return r_schemas.from_json(response.json(), r_schemas.QueueReactionsSchema())

    def check_exists(self, queue_identifier):
        """
        :type queue_identifier: r_objs.QueueIdentifier
        :rtype: bool
        """
        try:
            self.get(queue_identifier)
        except client_base.NoNamespaceError:
            return False
        else:
            return True

    def update(self,
               queue_identifier,
               new_queue_capacity=None,
               remove_reactions=None,
               add_reactions=None,
               new_max_instances_in_queue=None,
               new_max_instances_per_reaction_in_queue=None,
               new_max_running_instances_per_reaction=None):
        """
        :type queue_identifier: r_objs.QueueIdentifier
        :type new_queue_capacity: int
        :type remove_reactions: list[r_objs.OperationIdentifier]
        :type add_reactions: list[r_objs.OperationIdentifier]
        :type new_max_instances_in_queue: int
        :type new_max_instances_per_reaction_in_queue: int
        :type new_max_running_instances_per_reaction: int
        :return: {}
        """
        data = r_schemas.to_json(r_objs.QueueUpdateRequest(
            queue_identifier=queue_identifier,
            new_queue_capacity=new_queue_capacity,
            remove_reactions=remove_reactions,
            add_reactions=add_reactions,
            new_max_instances_in_queue=new_max_instances_in_queue,
            new_max_instances_per_reaction_in_queue=new_max_instances_per_reaction_in_queue,
            new_max_running_instances_per_reaction=new_max_running_instances_per_reaction),
            r_schemas.QueueUpdateRequestSchema())

        response = self.requester.send_request(
            "q/update",
            json=data,
            method="post"
        )

        return response.json()


class MetricEndpoint(ClientEndpoint, client_base.MetricEndpointBase):
    def create(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, custom_tags=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type custom_tags: dict[str, str]
        :rtype: r_objs.MetricReference
        """
        request = r_schemas.to_json(r_objs.MetricCreateRequest(
            metric_type=metric_type,
            artifact_id=artifact_id,
            queue_id=queue_id,
            reaction_id=reaction_id,
            custom_tags=r_objs.Attributes(custom_tags) if custom_tags is not None else None,
        ), r_schemas.MetricCreateRequestSchema())

        response = self.requester.send_request(
            "metric/create",
            json={"metric": request},
            method="post"
        )

        return r_schemas.from_json(response.json()["metric"], r_schemas.MetricReferenceSchema())

    def list(self, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :rtype: list[r_objs.MetricReference]
        """
        request = r_schemas.to_json(
            r_objs.MetricListRequest(artifact_id, queue_id, reaction_id), r_schemas.MetricListRequestSchema()
        )

        response = self.requester.send_request(
            "metric/list",
            json=request,
            method="post"
        ).json()

        return [
            r_schemas.from_json(r, r_schemas.MetricReferenceSchema())
            for r in response["metrics"]
        ]

    def update(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, new_tags=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type new_tags: dict | None
        :rtype: r_objs.MetricReference
        """
        request = r_schemas.to_json(
            r_objs.MetricUpdateRequest(
                metric_type=metric_type,
                artifact_id=artifact_id,
                queue_id=queue_id,
                reaction_id=reaction_id,
                new_tags=r_objs.Attributes(new_tags) if new_tags is not None else None,
            ),
            r_schemas.MetricUpdateRequestSchema())

        response = self.requester.send_request(
            "metric/update",
            json=request,
            method="post"
        )

        return r_schemas.from_json(response.json()["metric"], r_schemas.MetricReferenceSchema())

    def delete(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :return: {}
        """
        request = r_schemas.to_json(
            r_objs.MetricDeleteRequest(metric_type, artifact_id, queue_id, reaction_id),
            r_schemas.MetricDeleteRequestSchema()
        )

        response = self.requester.send_request(
            "metric/delete",
            json=request,
            method="post"
        )

        return response.json()


class NamespacePermissionEndpoint(
    ClientEndpoint, client_base.NamespacePermissionEndpointBase
):
    def change(self, namespace, revoke=None, grant=None, version=0):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :type revoke: List[str] | None
        :type grant: r_objs.NamespacePermissions | None
        :type version: int = 0
        :rtype: {}
        """
        data = r_schemas.to_json(
            namespace,
            r_schemas.NamespaceIdentifierSchema()
        )
        revoke_key = (
            'rolesToRevokePermissions'
            if version else 'loginsToRevokePermissions'
        )
        grant = r_schemas.to_json(
            grant, r_schemas.NamespacePermissionsSchema()
        )
        return self.requester.send_request(
            'n/permission/change',
            json={
                'namespace': data,
                revoke_key: revoke,
                'permissionsToGrant': grant
            },
            method='post'
        ).json()

    def list(self, namespace, version=0):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :type version: int = 0
        :rtype: r_objs.NamespacePermissions
        """
        data = r_schemas.to_json(
            namespace,
            r_schemas.NamespaceIdentifierSchema()
        )
        response = self.requester.send_request(
            'n/permission/list',
            json={'namespace': data, 'version': version},
            method='post'
        )
        return r_schemas.from_json(
            response.json()['permissions'],
            r_schemas.NamespacePermissionsSchema()
        )


class QuotaEndpoint(ClientEndpoint, client_base.QuotaEndpointBase):
    def get(self, namespace):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :rtype: r_objs.CleanupStrategyDescriptor
        """
        namespace = r_schemas.to_json(
            namespace,
            r_schemas.NamespaceIdentifierSchema()
        )

        response = self.requester.send_request(
            'quota/cleanupStrategy/get',
            json={'namespaceIdentifier': namespace},
            method='post'
        )

        if not response.json():
            return
        return r_schemas.from_json(
            response.json()['cleanupStrategy'],
            r_schemas.CleanupStrategyDescriptorSchema()
        )

    def update(self, namespace, cleanup_strategy):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :type cleanup_strategy: r_objs.CleanupStrategyDescriptor
        :rtype: {}
        """
        namespace = r_schemas.to_json(
            namespace,
            r_schemas.NamespaceIdentifierSchema()
        )
        cleanup_strategy = r_schemas.to_json(
            cleanup_strategy,
            r_schemas.CleanupStrategyDescriptorSchema()
        )
        response = self.requester.send_request(
            'quota/cleanupStrategy/update',
            json={
                'namespaceIdentifier': namespace,
                'cleanupStrategy': cleanup_strategy
            },
            method='post'
        )
        return response.json()

    def delete(self, namespace):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :rtype: {}
        """
        namespace = r_schemas.to_json(
            namespace,
            r_schemas.NamespaceIdentifierSchema()
        )
        response = self.requester.send_request(
            'quota/cleanupStrategy/delete',
            json={'namespaceIdentifier': namespace},
            method='post'
        )
        return response.json()


class CalculationEndpoint(ClientEndpoint, client_base.CalculationEndpointBase):
    def create(self, calculation, mappings=None, inactive=False):
        """
        :type calculation: r_objs.CalculationDescriptor
        :type mappings: list[YtPathMappingRule] | None
        :type inactive: bool
        :rtype: r_objs.CalculationReference
        """
        calculation = r_schemas.to_json(
            calculation,
            r_schemas.CalculationDescriptorSchema()
        )

        if mappings is not None:
            mappings = [
                r_schemas.to_json(
                    mapping,
                    r_schemas.YtPathMappingRuleSchema()
                )
                for mapping in mappings
            ]

        response = self.requester.send_request(
            'regular/calculation/create',
            json={
                'regularCalculation': calculation,
                'mappings': mappings,
                'inactive': inactive
            },
            method='post'
        )

        return r_schemas.from_json(response.json(), r_schemas.CalculationReferenceSchema())

    def get(self, root_namespace):
        """
        :type root_namespace: r_objs.NamespaceIdentifier
        :rtype: r_objs.Calculation
        """
        namespace = r_schemas.to_json(
            root_namespace,
            r_schemas.NamespaceIdentifierSchema()
        )

        response = self.requester.send_request(
            'regular/calculation/get',
            json={'folderIdentifier': namespace},
            method='post'
        )

        return r_schemas.from_json(response.json(), r_schemas.CalculationSchema())

    def list_metadata(self, root_namespaces):
        """
        :type root_namespaces: list[r_objs.NamespaceIdentifier]
        :rtype: dict[int, r_objs.CalculationRuntimeMetadata]
        """
        namespaces = [
            r_schemas.to_json(
                namespace,
                r_schemas.NamespaceIdentifierSchema()
            )
            for namespace in root_namespaces
        ]

        response = self.requester.send_request(
            'regular/calculation/listMetaData',
            json={'folderIdentifiers': namespaces},
            method='post'
        )

        return {
            folder_id: r_schemas.from_json(metadata, r_schemas.CalculationRuntimeMetadataSchema())
            for folder_id, metadata in response.json()['folderId2metaData'].items()
        }

    def update(self, calculation, version, mappings=None):
        """
        :type calculation: r_objs.CalculationDescriptor
        :type version: int
        :type mappings: list[YtPathMappingRule] | None
        :rtype: r_objs.CalculationReference
        """
        calculation = r_schemas.to_json(
            calculation,
            r_schemas.CalculationDescriptorSchema()
        )

        if mappings is not None:
            mappings = [
                r_schemas.to_json(
                    mapping,
                    r_schemas.YtPathMappingRuleSchema()
                )
                for mapping in mappings
            ]

        response = self.requester.send_request(
            'regular/calculation/update',
            json={
                'regularCalculation': calculation,
                'version': str(version),
                'mappings': mappings
            },
            method='post'
        )

        return r_schemas.from_json(response.json(), r_schemas.CalculationReferenceSchema())

    def delete(self, root_namespace):
        """
        :type root_namespace: r_objs.NamespaceIdentifier
        :rtype: {}
        """
        namespace = r_schemas.to_json(
            root_namespace,
            r_schemas.NamespaceIdentifierSchema()
        )

        response = self.requester.send_request(
            'regular/calculation/delete',
            json={'folderIdentifier': namespace},
            method='post'
        )

        return response.json()

    def translate(self, yt_paths, root_namespace=None, mappings=None):
        """
        :type yt_paths: list[str]
        :type root_namespace: list[r_objs.NamespaceIdentifier] | None
        :type mappings: list[r_objs.YtPathMappingRule] | None
        :rtype: list[r_objs.YtPathMappingCandidates]
        """
        if root_namespace is not None:
            root_namespace = r_schemas.to_json(
                root_namespace,
                r_schemas.NamespaceIdentifierSchema()
            )

        if mappings is not None:
            mappings = [
                r_schemas.to_json(
                    mapping,
                    r_schemas.YtPathMappingRuleSchema()
                )
                for mapping in mappings
            ]

        response = self.requester.send_request(
            'regular/calculation/translate',
            json={
                'ytPaths': yt_paths,
                'folderIdentifier': root_namespace,
                'mappings': mappings,
            },
            method='post'
        )

        return [
            r_schemas.from_json(yt_path_candidates, r_schemas.YtPathMappingCandidatesSchema())
            for yt_path_candidates in response.json()['mappings']
        ]


class ReactorAPIClientV1(client_base.ReactorAPIClientBase):
    API_PATH = "api/v1"

    def __init__(
        self,
        base_url,
        token,
        retry_policy=None,
        timeout=ReactorRequester.DEFAULT_TIMEOUT,
        handle_ts_zone=False,
    ):
        """
        :type base_url: str
        :type token: str
        :param retry_policy: No retries if None
        :type retry_policy: RetryPolicy | None
        :type handle_ts_zone: boolean
        """
        self._req_maker = ReactorRequester(base_url + "/" + self.API_PATH, token, retry_policy, timeout, handle_ts_zone)
        self._artifact_endpoint = ArtifactEndpoint(self._req_maker)
        self._artifact_type_endpoint = ArtifactTypeEndpoint(self._req_maker)
        self._artifact_trigger_endpoint = ArtifactTriggerEndpoint(self._req_maker)
        self._artifact_instance_endpoint = ArtifactInstanceEndpoint(self._req_maker)
        self._reaction_endpoint = ReactionEndpoint(self._req_maker)
        self._reaction_instance_endpoint = ReactionInstanceEndpoint(self._req_maker)
        self._namespace_endpoint = NamespaceEndpoint(self._req_maker)
        self._reaction_type_endpoint = ReactionTypeEndpoint(self._req_maker)
        self._namespace_notification_endpoint = NamespaceNotificationEndpoint(self._req_maker)
        self._queue_endpoint = QueueEndpoint(self._req_maker)
        self._metric_endpoint = MetricEndpoint(self._req_maker)
        self._quota_endpoint = QuotaEndpoint(self._req_maker)
        self._dynamic_trigger_endpoint = DynamicTriggerEndpoint(self._req_maker)
        self._calculation_endpoint = CalculationEndpoint(self._req_maker)
        self._permission_endpoint = NamespacePermissionEndpoint(self._req_maker)

    @property
    def artifact(self):
        return self._artifact_endpoint

    @property
    def artifact_type(self):
        return self._artifact_type_endpoint

    @property
    def artifact_trigger(self):
        return self._artifact_trigger_endpoint

    @property
    def artifact_instance(self):
        return self._artifact_instance_endpoint

    @property
    def reaction(self):
        return self._reaction_endpoint

    @property
    def namespace(self):
        return self._namespace_endpoint

    @property
    def requester(self):
        return self._req_maker

    @property
    def reaction_type(self):
        return self._reaction_type_endpoint

    @property
    def namespace_notification(self):
        return self._namespace_notification_endpoint

    @property
    def reaction_instance(self):
        return self._reaction_instance_endpoint

    @property
    def queue(self):
        return self._queue_endpoint

    @property
    def metric(self):
        return self._metric_endpoint

    @property
    def quota(self):
        return self._quota_endpoint

    @property
    def dynamic_trigger(self):
        return self._dynamic_trigger_endpoint

    @property
    def calculation(self):
        return self._calculation_endpoint

    @property
    def permission(self):
        return self._permission_endpoint

    def greet(self):
        response = self.requester.send_request(
            "u/greet",
            method="get"
        )

        return r_schemas.from_json(response.json(), r_schemas.GreetResponseSchema())
