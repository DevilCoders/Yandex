class TestClassForImportTest(object):
    def __init__(self):
        pass

    def __call__(self, value):
        return value


class SignatureSample(object):
    def __init__(self, ctor_one=100500, ctor_two="two_value", ctor_three=None):
        assert ctor_one, ctor_two
        assert ctor_three is None

    @staticmethod
    def sample_method(method_one="qwerty", method_two=1234.5678, method_three=True):
        return method_one, method_two, method_three
