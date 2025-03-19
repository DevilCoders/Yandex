import os
import sys
import argparse
import traceback

import gunicorn.app.base
import gunicorn.glogging
import django.core.wsgi

import metrika.pylib.config as lib_config


class Logger(gunicorn.glogging.Logger):
    def __init__(self, *args, **kwargs):
        super(Logger, self).__init__(*args, **kwargs)
        self.error_log.propagate = True
        self.access_log.propagate = True

    def access(self, resp, req, environ, request_time):
        headers = {k.lower(): v for k, v in req.headers if k.lower().startswith('x-')}
        res = {
            'http_user_agent': environ.get('HTTP_USER_AGENT'),
            'http_referrer': environ.get('HTTP_REFERER'),
            'src_ip': environ.get('HTTP_X_FORWARDED_FOR') or environ.get('REMOTE_ADDR'),
            'status': resp.status_code,
            'byte': resp.sent,
            'request_time': request_time.microseconds,
            'x-headers': headers,
        }

        self.access_log.info('{} {} {}'.format(req.method, req.uri, environ.get('SERVER_PROTOCOL')), res)


class Application(gunicorn.app.base.BaseApplication):
    def __init__(self, config_file):
        os.environ.setdefault("DJANGO_SETTINGS_MODULE", 'cloud.mdb.backstage.settings.settings')
        os.environ.setdefault("BACKSTAGE_CONFIG_FILE", config_file)
        self.__config = lib_config.get_yaml_config_from_file(config_file)
        self.application = django.core.wsgi.get_wsgi_application()
        super(Application, self).__init__()

    def load(self):
        return self.application

    def load_config(self):
        try:
            for key, value in self.__config['gunicorn'].items():
                self.cfg.set(key, value)

            self.cfg.set('logger_class', Logger)
        except Exception as err:
            tb = traceback.format_exc()
            sys.stderr.write('Failed to load config: {}. {}'.format(err, tb))
            raise


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', required=True)
    args = parser.parse_args()

    Application(config_file=args.config).run()


if __name__ == '__main__':
    main()
