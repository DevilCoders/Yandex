# for unit tests and 'pure-functional' serp parser
class DummyLock:
    def __init__(self):
        pass

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass
