from .spu import SPU


class SSPU(SPU):
    def __init__(self, minutes=30):
        super(SSPU, self).__init__(minutes=minutes)

    def __call__(self, experiment):
        sessions = super(SSPU, self).__call__(experiment)
        if sessions > 1:
            return sessions
