"""Gencfg internal error types"""


class IGencfgException(Exception):
    def __init__(self, msg):
        super(IGencfgException, self).__init__(msg)

    def dict_params(self):
        raise NotImplementedError('Subclasses of IGencfgException must override dict_prams() method')


class TValidateCardNodeError(IGencfgException):
    def __init__(self, msg):
        super(TValidateCardNodeError, self).__init__(msg)

        self.msg = msg

    def dict_params(self):
        return {
            'msg': self.msg,
        }


class UtilNormalizeException(IGencfgException):
    def __init__(self, util, params, message):
        super(UtilNormalizeException, self).__init__(
            "Incorrect params combination <%s> in util <%s>: %s" % (",".join(params), util, message))

        self.util = util
        self.params = params
        self.message = message

    def dict_params(self):
        return {
            'util': self.util,
            'params': self.params,
            'message': self.message,
        }


class UtilRuntimeException(IGencfgException):
    def __init__(self, util, message):
        super(UtilRuntimeException, self).__init__("Util <%s> raised runtime exception: %s" % (util, message))

        self.util = util
        self.message = message

    def dict_params(self):
        return {
            'util': self.util,
            'message': self.message,
        }


class TInstanceNotFoundException(IGencfgException):
    def __init__(self, instance_str):
        super(TInstanceNotFoundException, self).__init__(
            "Not found instance <%s> while getting instance from ghi" % instance_str)

        self.instance_str = instance_str

    def dict_params(self):
        return {
            'instance_str': self.instance_str,
        }

class TInstanceAlreadyInGHI(IGencfgException):
    def __init__(self, instance_str, already_in_ghi_str, group_name):
        super(TInstanceAlreadyInGHI, self).__init__("Instance <{}> already in ghi (as <{}>) when adding to group <{}>".format(
                                                    instance_str, already_in_ghi_str, group_name))
        self.instance_str = instance_str

    def dict_params(self):
        return {
            'instance_str': self.instance_str,
            'group_name': group_name,
        }
