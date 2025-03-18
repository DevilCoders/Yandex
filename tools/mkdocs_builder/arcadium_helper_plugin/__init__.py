import logging
import os

from mkdocs import plugins
from mkdocs.config import base as config_base

logger = logging.getLogger(__name__)


class Helper(plugins.BasePlugin):

    def on_config(self, config):
        theme = config.data['theme']
        if theme.name == 'material_yandex' and 'templates_dir' in theme._vars:
            templates_dir = os.path.join(config['docs_dir'], theme._vars.pop('templates_dir'))
            if not os.path.isdir(templates_dir):
                raise config_base.ValidationError('The "templates_dir" path for theme {} does not exists in docs_dir.'.format(theme.name))

            theme.dirs.insert(0, templates_dir)
