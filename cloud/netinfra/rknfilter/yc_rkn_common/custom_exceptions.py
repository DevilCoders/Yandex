class input_error_exception(Exception):
    def __init__(self, message):
        super().__init__(message)


class configuration_eligibility_exception(Exception):
    def __init__(self, message):
        super().__init__(message)


class nothing_to_do_exception(Exception):
    def __init__(self, message):
        super().__init__(message)


class reconfiguration_exception(Exception):
    def __init__(self, message):
        super().__init__(message)


class rkn_api_communication_error(Exception):
    def __init__(self, message):
        super().__init__(message)


class ripe_api_communication_error(Exception):
    def __init__(self, message):
        super().__init__(message)


class config_node_core_workflow_exception(Exception):
    def __init__(self, message):
        super().__init__(message)
