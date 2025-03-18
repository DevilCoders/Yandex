from xml.etree import cElementTree


class ConfigParams(object):
    def __init__(self, tree):
        self.params = {}
        for elem in tree:
            self.params[elem.tag] = elem.text.strip()

    def __getitem__(self, name):
        return self.params[name]


config_params = None


def init_config_params(tree):
    global config_params

    if tree.find('params'):
        config_params = ConfigParams(tree.find('params'))
    else:
        config_params = ConfigParams(cElementTree.Element('root'))


def get_config_params():
    global config_params

    if config_params is None:
        raise Exception("Asking for config_params, which is not initialized yet")

    return config_params
