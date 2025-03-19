"""
Mock module with fake classes
"""

FAKE_TASK = {'task_id': None, 'feature_flags': [], 'folder_id': '', 'context': {}}


# pylint: disable=too-few-public-methods
class DoNothing:
    """
    Class that implements do nothing behaviour
    All methods do nothing
    """
    def do_nothing(self, *args, **kwargs):
        """
        Method which does nothing
        """
    def __call__(self, *args, **kwargs):
        return self.do_nothing(*args, **kwargs)

    def __getattribute__(self, name):
        if name == 'do_nothing':
            return object.__getattribute__(self, name)
        return self.do_nothing
