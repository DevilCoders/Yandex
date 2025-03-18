"""
    Class with all common settings, like urls to nanny/sandbox/bot/... , default timeouts, default methods to get users list and etc.
"""

import os

from core.card.node import load_card_node


class TSettings(object):
    """
        Class with all code settings
    """

    def __init__(self):
        self._initialized = False

    def _init(self):
        settings_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'settings')
        scheme_file = os.path.join(settings_dir, 'scheme.yaml')
        data_file = os.path.join(settings_dir, 'settings.yaml')
        contents = load_card_node(data_file, schemefile=scheme_file)

        self.__dict__.update(contents.__dict__)

        self._initialized = True

    def __getattr__(self, name):
        if not self._initialized:
            self._init()

        return self.__dict__[name]


SETTINGS = TSettings()


class TDbSettings(object):
    """
        Class with various db settings (some things like list of locations for dynamic should be listed here)
    """

    def __init__(self, db):
        self.db = db

        if self.db.version <= "2.2.24":
            settings_file = os.path.join(os.path.dirname(os.path.dirname(__file__)), "legacy", "settings.yaml")
            scheme_file = os.path.join(os.path.dirname(os.path.dirname(__file__)), "legacy", "settings_scheme.yaml")
        else:
            settings_file = os.path.join(db.get_path(), "settings.yaml")
            scheme_file = os.path.join(db.get_path(), "schemes", "settings.yaml")

        contents = load_card_node(settings_file, schemefile=scheme_file)
        assert getattr(contents, "db", None) is None

        self.__dict__.update(contents.__dict__)

    def update(self, smart=False):
        pass  # nothing to do

    def fast_check(self, timeout):
        pass  # nothing to do
