from enum import Enum


JOB_STATUS_ORDER = {
    'DECLINED': 0,
    'ACCEPTED': 1,
    'DOWNLOADING_NIRVANA_BUNDLE': 2,
    'PULLING_PORTO_LAYERS': 3,
    'CONVERTING_PORTO_LAYERS': 4,
    'PUT_DOCKER_IMAGE_TO_CACHE': 5,
    'GET_CACHED_DOCKER_IMAGE': 6,
    'BUILDING_IMAGE': 7,
    'RUNNING': 8,
    'STOPPED': 9,
    'FAILED': 10,
    'AGENT_UNAVAILABLE': 11,
    'COMPLETED': 12
}


class JobStatus(Enum):
    ACCEPTED = 'ACCEPTED'
    DECLINED = 'DECLINED'
    DOWNLOADING_NIRVANA_BUNDLE = 'DOWNLOADING_NIRVANA_BUNDLE'
    PULLING_PORTO_LAYERS = 'PULLING_PORTO_LAYERS'
    CONVERTING_PORTO_LAYERS = 'CONVERTING_PORTO_LAYERS'
    PUT_DOCKER_IMAGE_TO_CACHE = 'PUT_DOCKER_IMAGE_TO_CACHE'
    GET_CACHED_DOCKER_IMAGE = 'GET_CACHED_DOCKER_IMAGE'
    BUILDING_IMAGE = 'BUILDING_IMAGE'
    RUNNING = 'RUNNING'
    STOPPED = 'STOPPED'
    FAILED = 'FAILED'
    AGENT_UNAVAILABLE = 'AGENT_UNAVAILABLE'
    COMPLETED = 'COMPLETED'

    def is_job_started(self):
        return self < JobStatus.RUNNING

    def is_job_in_process(self):
        return JobStatus.DOWNLOADING_NIRVANA_BUNDLE <= self <= JobStatus.RUNNING

    def __lt__(self, other):
        return JOB_STATUS_ORDER[self.value] < JOB_STATUS_ORDER[other.value]

    def __le__(self, other):
        return JOB_STATUS_ORDER[self.value] <= JOB_STATUS_ORDER[other.value]

    def __gt__(self, other):
        return not self <= other

    def __ge__(self, other):
        return not self < other


if __name__ == '__main__':
    assert JobStatus.ACCEPTED > JobStatus.DECLINED
    assert JobStatus.DECLINED < JobStatus.ACCEPTED
    assert JobStatus.ACCEPTED >= JobStatus.DECLINED
    assert JobStatus.DECLINED <= JobStatus.ACCEPTED
    assert JobStatus.ACCEPTED < JobStatus.CONVERTING_PORTO_LAYERS
    assert JobStatus.CONVERTING_PORTO_LAYERS < JobStatus.BUILDING_IMAGE
    assert JobStatus.BUILDING_IMAGE < JobStatus.RUNNING
    assert JobStatus.RUNNING < JobStatus.STOPPED
    assert JobStatus.STOPPED < JobStatus.FAILED
    assert JobStatus.FAILED < JobStatus.COMPLETED
