from yaqutils import YaqEnum


class OfflineCalcMode(YaqEnum):
    AUTO = "auto"
    LOCAL = "local"
    YT = "yt"

    ALL = {
        AUTO,
        LOCAL,
        YT
    }


class ModesEnum(YaqEnum):
    AVG = "avg"
    BEST_RATE = "best-rate"
    COVERAGE = "coverage"
    DCG = "dcg"
    DEFECT_RATE = "defect-rate"
    DEFECT_RATE_DCG = "defect-rate-dcg"
    SUM = "sum"
    ALL = {AVG, BEST_RATE, COVERAGE, DCG, DEFECT_RATE, DEFECT_RATE_DCG, SUM}
