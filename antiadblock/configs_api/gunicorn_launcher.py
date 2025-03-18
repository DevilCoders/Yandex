from gunicorn.app.base import BaseApplication

import configs_api.config_gunicorn as config


class ConfigsAPIGunicornApp(BaseApplication):

    def init(self, parser, opts, args):
        pass

    def __init__(self):
        super(ConfigsAPIGunicornApp, self).__init__()

    def load_config(self):
        self.cfg.set("default_proc_name", "configs_api")

        for option in [option for option in dir(config) if not option.startswith('_')]:
            self.cfg.set(option, getattr(config, option))

    def load(self):
        import configs_api.app
        return configs_api.app.app


def main():
    ConfigsAPIGunicornApp().run()


if __name__ == "__main__":
    main()
