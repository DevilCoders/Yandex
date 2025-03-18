class BucketMetric(object):
    def __call__(self, experiment):
        return list(set(action.get("bucket", -1) for action in experiment.actions))
