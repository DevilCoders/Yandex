# coding=utf-8

# В результате серии экспериментов был выбран worker_class = sync,
# так как в результате использования нескольких курсоров БД при использовании
# gevent потоков могут возникнуть дедлоки в БД
# http://docs.sqlalchemy.org/en/latest/errors.html#error-3o7r
# http://initd.org/psycopg/docs/advanced.html?highlight=green%20threads#support-for-coroutine-libraries


workers = 2  # multiprocessing.cpu_count() is 56(32) for any qloud flavour
worker_class = 'sync'
