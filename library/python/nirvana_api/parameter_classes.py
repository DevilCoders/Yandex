# [doc types]
from datetime import datetime  # noqa

import enum
import six


class BlockEndpointPromotionData(object):

    def __init__(self, endpoint, name, description=None, quota_project=None):
        self.endpoint = endpoint
        self.name = name
        self.description = description
        self.quotaProject = quota_project


class EndpointType(object):
    input = 'input'
    output = 'output'


class AddOperationInputParams(object):

    def __init__(self, name, data_types, lower_bound=None, upper_bound=None, description=None):
        self.name = name
        self.type = EndpointType.input
        self.dataTypes = data_types
        self.lowerBound = lower_bound
        self.upperBound = upper_bound
        self.description = description


class AddOperationOutputParams(object):

    def __init__(self, name, data_type, description=None, optional=False, snapshot=False):
        self.name = name
        self.type = EndpointType.output
        self.dataType = data_type
        self.description = description
        self.optional = optional
        self.snapshot = snapshot


class OperationEndpointReference(object):

    def __init__(self, name, type):
        self.name = name
        self.type = type


class OperationBlock(object):

    def __init__(self, operation_id, name, code=None):
        self.operationId = operation_id
        self.name = name
        self.code = code


class DataBlock(object):

    def __init__(self, stored_data_id, name, code=None):
        self.storedDataId = stored_data_id
        self.name = name
        self.code = code


class EditBlock(object):

    def __init__(self, name=None, code=None, data_id=None):
        self.name = name
        self.blockCode = code
        self.dataId = data_id


class BlockPosition(object):
    def __init__(self, x=None, y=None, width=None, height=None):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


class Parameter(object):
    def __repr__(self):
        return '{!s}({!s})'.format(
            self.__class__.__name__, ', '.join('{!s}={!r}'.format(k, v) for k, v in six.iteritems(self.__dict__))
        )


class BlockPattern(Parameter):

    def __init__(self, guid=None, code=None):
        self.guid = guid
        self.code = code

    def __eq__(self, other):
        return isinstance(other, BlockPattern) and self.guid == other.guid and self.code == other.code

    def __hash__(self):
        return hash((self.guid, self.code))


class ParameterValue(Parameter):

    def __init__(self, parameter, value=None):
        self.parameter = parameter
        self.value = value


class ComputableParameterValue(ParameterValue):

    def __init__(self, parameter, value=None, expressions=None):
        super(ComputableParameterValue, self).__init__(parameter, value)
        self.expressions = expressions


class Permissions(object):

    def __init__(self, action, role):
        self.action = action
        self.role = role


class ParameterReference(object):

    def __init__(self, parameter, derived_from):
        self.parameter = parameter
        self.derivedFrom = derived_from


class GlobalParameter(object):

    def __init__(self, parameter, parameter_type, extra_enum_items=None, description=None, default_value=None):
        self.parameter = parameter
        self.type = parameter_type
        self.extra = dict(items=extra_enum_items) if extra_enum_items else None
        self.description = description
        self.value = default_value

    def __json__(self):
        return dict(parameter=self.parameter, type=self.type, extra=self.extra, description=self.description)


class Expressions(list):
    pass


class AddOperationParameterParams(object):

    def __init__(self, parameter, parameter_type, extra_enum_items=None, description=None, default_value=None, label=None, required=None, cache_insensitive=None):
        self.parameter = parameter
        self.type = parameter_type
        self.extra = dict(items=extra_enum_items) if extra_enum_items else None
        self.description = description
        self.defaultValue = default_value
        self.label = label
        self.required = required
        self.cacheInsensitive = cache_insensitive


class EnumItem(object):

    def __init__(self, label, value):
        self.label = label
        self.value = value


class WorkflowExecutionParams(object):

    def __init__(self, priority=None, error_handling_policy=None, concurrent_operations_count=None):
        self.workflowPriority = priority
        self.errorHandlingPolicy = error_handling_policy
        self.concurrentOperationsCount = concurrent_operations_count


class StrEnum(enum.Enum):
    def __repr__(self):
        return '<{!s}.{!s}>'.format(self.__class__.__name__, self.name)


class StatusEnum(StrEnum):
    WAITING = "waiting"
    RUNNING = "running"
    COMPLETED = "completed"
    UNDEFINED = "undefined"


class ResultEnum(StrEnum):
    SUCCESS = "success"
    FAILURE = "failure"
    CANCEL = "cancel"
    UNDEFINED = "undefined"


class WorkflowFilters(Parameter):

    def __init__(self, quota_project_id=None, rejected=None, tags=None, author=None,
                 created=None, started=None, completed=None, status=None, result=None, ns_path=None, ns_id=None):
        """
        :type quota_project_id: str|None
        :type rejected: bool|None
        :type tags: list[str]|None
        :type author: list[str]|None
        :type created: DateRange|None
        :type started: DateRange|None
        :type completed: DateRange|None
        :type status: list[StatusEnum]|None
        :type result: list[ResultEnum]|None
        """
        self.quotaProjectId = quota_project_id
        self.rejected = rejected
        self.tags = tags
        self.author = author
        self.created = created
        self.started = started
        self.completed = completed
        self.status = [s.value for s in status] if status is not None else None
        self.result = [r.value for r in result] if result is not None else None
        self.nsPath = ns_path
        self.nsId = ns_id


class WorkflowPriority(object):
    high = 'high'
    normal = 'normal'
    low = 'low'


class ErrorHandlingPolicy(object):
    stop = 'stop'
    ignore = 'ignore'


class PaginationData(Parameter):
    MIN_PAGE_NUMBER = 1
    MIN_PAGE_SIZE = 1
    MAX_PAGE_SIZE = 300

    def __init__(self, page_size, page_number):
        if not (self.MIN_PAGE_SIZE <= page_size <= self.MAX_PAGE_SIZE):
            raise ValueError('Bad page_size value. Should be number between {} and {}'.format(
                self.MIN_PAGE_SIZE, self.MAX_PAGE_SIZE))

        if page_number < self.MIN_PAGE_NUMBER:
            raise ValueError('Bad page_number value. '
                             'Should be number greater than or equal to {}'.format(self.MIN_PAGE_NUMBER))

        self.pageSize = page_size
        self.pageNumber = page_number


class EditOperationParameter(object):

    def __init__(self, parameter, new_parameter=None, parameter_type=None, required=None, default_value=None, label=None, description=None):
        self.parameter = parameter
        self.newProperties = OperationParameter(new_parameter, parameter_type, required, default_value, label, description)


class OperationParameter(object):

    def __init__(self, parameter=None, parameter_type=None, required=None, default_value=None, label=None, description=None):
        self.parameter = parameter
        self.type = parameter_type
        self.required = required
        self.defaultValue = default_value
        self.label = label
        self.description = description


class ParameterType(object):
    string = 'string'
    boolean = 'boolean'
    integer = 'integer'
    number = 'number'
    resource = 'resource'
    secret = 'secret'
    enum = 'enum'

    #  write-only parameter type
    #  all get-requests return string instead of date
    #  https://ml.yandex-team.ru/thread/nirvana/2370000004994812441/#message2370000004994812441
    date = 'date'

    multiple_strings = 'multiple_strings'
    multiple_integers = 'multiple_integers'
    multiple_numbers = 'multiple_numbers'
    multiple_enums = 'multiple_enums'


class ResourceType(object):
    static = 'static'
    dynamic = 'dynamic'


class DeprecateKind(object):
    optional = 'optional'
    mandatory = 'mandatory'


class ScpUploadParameters(object):

    def __init__(self, remote_path, archive=None):
        self.protocol = 'scp'
        self.remotePath = remote_path
        self.archive = archive


class HttpUploadParameters(object):

    def __init__(self, url, archive=None):
        self.protocol = 'http'
        self.url = url
        self.archive = archive


class SandboxUploadParameters(object):

    def __init__(self, arcadia_url, target_path, artifact_path, archive=None):
        self.protocol = 'sandbox'
        self.arcadiaUrl = arcadia_url
        self.targetPath = target_path
        self.artifactPath = artifact_path
        self.archive = archive


class SyncHttpUploadParameters(object):

    def __init__(self, archive=None):
        self.protocol = 'sync_http'
        self.archive = archive


class UploadMethods(object):
    SCP = 0
    HTTP = 1
    SYNC_HTTP = 2
    SANDBOX = 3


class TagList(Parameter):
    def __init__(self, names, strict_tag_check):
        """
        :type names: list[str]
        :type strict_tag_check: boolean
        """
        self.names = names
        self.strictTagCheck = strict_tag_check


class DateRange(Parameter):
    def __init__(self, from_date=None, to_date=None):
        """
        :type from_date: datetime|None
        :type to_date: datetime|None
        """
        self.from_ = None
        self.to = None
        if from_date is not None:
            self.from_ = from_date.strftime('%Y-%m-%dT%H:%M:%S+0300')
        if to_date is not None:
            self.to = to_date.strftime('%Y-%m-%dT%H:%M:%S+0300')
