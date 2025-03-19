"""
Events utils
"""

from datetime import datetime
from typing import Any, Dict, Optional

from ..core import id_generators
from ..core.types import Operation, OperationEvent
from .register import get_operations_describer
from .request_context import (
    get_auth_context,
    get_cloud_ext_id_from_request,
    get_folder_ids_from_request,
    get_idempotence_from_request,
    get_x_request_id,
    get_forwarded_remote_address,
    get_forwarded_user_agent,
    get_user_request,
)
from .time import datetime_to_rfc3339_utcoffset
from .types import IdempotenceData

JSDict = Dict[str, Any]


def render_authentication(auth_ctx: Dict[str, Any]) -> JSDict:
    """
    Render message Authentication {
      enum SubjectType {
        SUBJECT_TYPE_UNSPECIFIED = 0;
        YANDEX_PASSPORT_USER_ACCOUNT = 1;
        SERVICE_ACCOUNT = 2;
      }
      bool authenticated = 1;
      SubjectType subject_type = 2;
      string subject_id = 3;
    }
    """
    try:
        user_type = auth_ctx['authentication']['user_type']
    except (KeyError, TypeError) as exc:
        raise RuntimeError('There are no authorization.user_type in auth context') from exc

    subject_type = {
        'user_account': 'YANDEX_PASSPORT_USER_ACCOUNT',
        'service_account': 'SERVICE_ACCOUNT',
    }.get(user_type, 'SUBJECT_TYPE_UNSPECIFIED')

    return {
        'authenticated': True,
        'subject_type': subject_type,
        'subject_id': auth_ctx['user_id'],
    }


def render_authorization(auth_ctx: Dict[str, Any]) -> JSDict:
    """
    Render message Authorization {
      bool authorized = 1;
      repeated RequestedPermissions permissions = 2;
    }

    message RequestedPermissions {
      // <service>.<resource>.<action>
      string permission = 1 [(required) = true];
      // <service>.<resource>
      string resource_type = 2 [(required) = true];
      string resource_id = 3 [(required) = true];
      // is request for permission authorized
      bool authorized = 4;
    }
    """
    try:
        authorizations = auth_ctx['authorizations']
    except (KeyError, TypeError) as exc:
        raise RuntimeError('There are no authorization info in auth context') from exc

    try:
        perms = [
            {
                'permission': d['action'],
                'resource_type': 'resource-manager.' + d['entity_type'],
                'resource_id': d['entity_id'],
                'authorized': True,
            }
            for d in authorizations
        ]
    except (KeyError, TypeError) as exc:
        raise RuntimeError('Malformed authorizations ctx') from exc

    return {
        'authorized': True,
        'permissions': perms,
    }


def render_request_metadata(
    idempotency: Optional[IdempotenceData],
    x_request_id: Optional[str],
    remote_address: Optional[str],
    user_agent: Optional[str],
) -> JSDict:
    """
    Render message RequestMetadata {
      string remote_address = 1 [(required) = true];
      string user_agent = 2 [(required) = true];
      string request_id = 3 [(required) = true];
      string idempotency_id = 4;
    }
    """
    request_metadata: JSDict = {
        'remote_address': remote_address or '',
        'user_agent': user_agent or '',
        'request_id': x_request_id or '',
    }
    # idempotency_id is optional,
    # so sent it only if it present
    if idempotency is not None:
        request_metadata['idempotency_id'] = idempotency.idempotence_id
    return request_metadata


def render_event_metadata(
    event_id: str, event_type: str, created_at: datetime, folder_ext_id: str, cloud_ext_id: str
) -> JSDict:
    """
    Render message EventMetadata {
      string event_id = 1 [(required) = true];
      string event_type = 2 [(required) = true];
      google.protobuf.Timestamp created_at = 3 [(required) = true];
      TracingContext tracing_context = 4;
      string cloud_id = 5 [(required) = true];
      string folder_id = 6;
    }

    message TracingContext {
      string trace_id = 1;
      string span_id = 2;
      string parent_span_id = 3;
    }
    """
    return {
        'event_id': event_id,
        'event_type': event_type,
        'created_at': datetime_to_rfc3339_utcoffset(created_at),
        'cloud_id': cloud_ext_id,
        'folder_id': folder_ext_id,
    }


def render_event(
    event: OperationEvent,
    event_id: str,
    auth_ctx: Dict[str, Any],
    folder_ext_id: str,
    cloud_ext_id: str,
    idempotence: Optional[IdempotenceData],
    x_request_id: Optional[str],
    created_at: datetime,
    remote_address: Optional[str],
    user_agent: Optional[str],
    operation_id: Optional[str],
    request_parameters: Optional[dict],
) -> JSDict:
    """
    Render message yandex.cloud.events.mdb.*.YourMessage {
        Authentication authentication = 1 [(required) = true];
        Authorization authorization = 2 [(required) = true];

        EventMetadata event_metadata = 3 [(required) = true];
        RequestMetadata request_metadata = 4 [(required) = true];

        EventStatus event_status = 5 [(required) = true];
        EventDetails details = 6 [(required) = true];

        google.rpc.Status error = 7;
        RequestParameters request_parameters = 8;
        events.Response response = 9;

        enum EventStatus {
            EVENT_STATUS_UNSPECIFIED = 0;
            STARTED = 1;
            DONE = 2;
            CANCELLED = 4;
        }

        message EventDetails {
            string cluster_id = 1 [(required) = true];
            // different fields for different messages
        }
        message Status {
            int32 code = 1;
            string message = 2;
            repeated google.protobuf.Any details = 3;
        }

        message Response {
            string operation_id = 1;
        }
    }
    """
    return {
        'authentication': render_authentication(auth_ctx),
        'authorization': render_authorization(auth_ctx),
        'event_metadata': render_event_metadata(
            event_id=event_id,
            event_type=event.event_type,
            created_at=created_at,
            folder_ext_id=folder_ext_id,
            cloud_ext_id=cloud_ext_id,
        ),
        'request_metadata': render_request_metadata(
            idempotency=idempotence,
            x_request_id=x_request_id,
            remote_address=remote_address,
            user_agent=user_agent,
        ),
        'event_status': 'EVENT_STATUS_UNSPECIFIED',
        'details': event.details,
        'error': None,  # FIXME
        'request_parameters': request_parameters,  # NOTE: shouldn't contain sensitive information
        'response': {'operation_id': operation_id},
    }


def make_event(operation: Operation) -> Optional[JSDict]:
    """
    Make event for given operation
    """

    event = get_operations_describer(operation.cluster_type).get_event(operation, get_user_request())
    if event is None:
        # looks like that operation, doesn't provide events
        return None

    event_id = id_generators.gen_id('event_id')

    # actually we have folder only in modification requests,
    # but right now we create events only for them
    folder_ext_id = get_folder_ids_from_request().folder_ext_id

    return render_event(
        event=event,
        event_id=event_id,
        auth_ctx=get_auth_context(),
        folder_ext_id=folder_ext_id,
        cloud_ext_id=get_cloud_ext_id_from_request(),
        idempotence=get_idempotence_from_request(),
        x_request_id=get_x_request_id(),
        created_at=operation.created_at,
        remote_address=get_forwarded_remote_address(),
        user_agent=get_forwarded_user_agent(),
        operation_id=operation.id,
        request_parameters=event.request_parameters,
    )
