class PluginSample(object):
    def __init__(self):
        pass


class PluginSampleOne(object):
    def __init__(self, single_param):
        assert single_param == 1


class PluginSampleTwo(object):
    def __init__(self, abc, xyz):
        assert abc == 100
        assert xyz == 500
