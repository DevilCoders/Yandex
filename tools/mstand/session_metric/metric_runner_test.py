class MetricNoRequiredServices(object):
    def __call__(self):
        return 1


class MetricWithRequiredServices(object):
    def __call__(self):
        return 1

    required_services = ["alice", "news", "taxi", "web"]
