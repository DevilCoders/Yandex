# -*- coding: utf-8 -*-

class ResourceAction(object):
    # for resources that do not require modification
    SKIP = "SKIP"
    # for resources that do require modification
    MODIFY = "MODIFY"
    CREATE = "CREATE"
    DELETE = "DELETE"
    # for obsolete resources
    IGNORE = "IGNORE"


MODIFYING_ACTIONS = {
    ResourceAction.MODIFY,
    ResourceAction.CREATE,
    ResourceAction.DELETE
}


class Resource(object):

    def __init__(
        self,
        local_id,
        local_state,
        provider_id=None,
        provider_state=None
    ):
        self.local_id = local_id
        self.local_state = local_state
        self.provider_id = provider_id
        self.provider_state = provider_state
        self.action = None
