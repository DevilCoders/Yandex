# noinspection PyClassHasNoInit
class CriteriaReadMode:
    FLOAT_1D = "1d"
    FLOAT_2D = "2d"
    JSON_LISTS = "lists"

    ALL = (FLOAT_1D, FLOAT_2D, JSON_LISTS)

    @staticmethod
    def from_criteria(criteria):
        read_mode = getattr(criteria, "read_mode", CriteriaReadMode.FLOAT_1D)
        if read_mode not in CriteriaReadMode.ALL:
            raise Exception("Unknown criteria read mode: {}".format(read_mode))
        return read_mode
