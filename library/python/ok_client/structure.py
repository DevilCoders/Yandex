import typing


class StageSimple(typing.TypedDict):
    approver: str


class StageComplex(typing.TypedDict):
    need_all: bool
    stages: typing.List[StageSimple]


class StageResponse(typing.TypedDict):
    id: int
    approver: str


class Actions(typing.TypedDict):
    approve: bool
    reject: bool
    suspend: bool
    resume: bool
    close: bool


class TrackedQueue(typing.TypedDict):
    name: str
    has_triggers: bool


class UserRoles(typing.TypedDict):
    responsible: bool
    approver: bool


class CreateApprovementRequest(typing.TypedDict, total=False):
    """
    https://wiki.yandex-team.ru/Intranet/OK/private-api/#sozdanieizapusksoglasovanija
    """
    type: str
    object_id: str
    text: str
    stages: typing.List[typing.Union[StageSimple, StageComplex]]
    author: str
    groups: typing.List[str]
    is_parallel: bool


class GetApprovementResponse(typing.TypedDict, total=False):

    id: int
    uuid: str
    stages: typing.List[StageResponse]
    author: str
    text: str
    is_parallel: bool
    url: str
    object_id: str
    status: str
    resolution: str
    actions: Actions
    created: str
    modified: str
    tracker_queue: TrackedQueue
    user_roles: UserRoles
