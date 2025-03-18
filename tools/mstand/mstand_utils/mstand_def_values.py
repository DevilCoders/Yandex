# defaults for offline part


class OfflineDefaultValues:
    FETCH_THREADS = 4
    CONVERT_THREADS = 4
    UNPACK_THREADS = 4
    DEF_YT_CLUSTER = "hahn"
    DEF_YT_POOL = "mstand-offline"
    DEF_YT_PATH = "//home/mstand-offline/prod"
    DEF_YT_OUTPUT_TTL = 86400 * 5
    # see resource_calculation_script.groovy
    LOCAL_MODE_MAX_METRICS = 8
    # see latest examples in MSTAND-1807
    SERPSET_UPLOAD_BUFFER_SIZE = 1400
    MIN_YT_JOB_COUNT = 20
    OPERATION_OWNERS = [
        "vdmit",
        "lucius",
        "splav",
        "serg-v",
        "ibrishat",
        "dergunov",
        "knyshovalex",
        "robot-metrics-srv",
        "robot-mstand"
    ]
