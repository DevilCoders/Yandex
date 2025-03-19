default_log_config = {'version': 1,
                      'disable_existing_loggers': False,
                      'formatters':
                      {'simple':
                       {'format': '%(asctime)s - %(name)s: [%(levelname)s]: %(message)s'}},
                      'handlers':  {
                          'console': {
                              'class': 'logging.StreamHandler',
                              'level': 'DEBUG',
                              'formatter': 'simple',
                              'stream': 'ext://sys.stdout'
                          },
                          'file': {
                              'level': 'DEBUG',
                              'formatter': 'simple',
                              'class': 'logging.handlers.RotatingFileHandler',
                              'filename': 'analytics.log',
                              'mode': 'a',
                              'maxBytes': 1048576,
                              'backupCount': 5
                          },
                      },

                      'loggers': {
                          'urllib3': {'level': 'ERROR'},
                          'numba': {'level': 'ERROR'},
                          'yql.client.request': {'level': 'INFO'},
                          'matplotlib': {'level': 'ERROR'},
                          'yt.packages.urllib3': {'level': 'ERROR'},
                          'zeep': {'level': 'ERROR'},
                          'py4j': {'level': 'ERROR'},
                          'nile': {'level': 'ERROR'},
                          'qb2': {'level': 'ERROR'},
                          'startrek_client': {'level': 'ERROR'},
                          'yandex_tracker_client': {'level': 'ERROR'},

                      },
                      'root': {'level': 'DEBUG',
                               'handlers': ['console', 'file']}
                      }
