from gunicorn.app.base import BaseApplication

import antiadblock.configs_api.bin.config_gunicorn as config
import os


class ConfigsAPIGunicornApp(BaseApplication):

    def init(self, parser, opts, args):
        pass

    def __init__(self):
        super(ConfigsAPIGunicornApp, self).__init__()

    def load_config(self):
        self.cfg.set("default_proc_name", "configs_api")
        self.cfg.set('bind', os.environ.get('APP_BIND_STRING', '[::]:80'))

        for option in [option for option in dir(config) if not option.startswith('_')]:
            self.cfg.set(option, getattr(config, option))

    def load(self):
        import antiadblock.configs_api.lib.app as app
        app.init_app()
        return app.app


def main():
    ConfigsAPIGunicornApp().run()


if __name__ == "__main__":
    main()
