import threading
import requests


def parallel(fun):
    '''
    A parallel run decorator
    '''
    def xxx(self, multiarg, *args, **kwargs):
        errors = []

        def tf (self, errors, multiarg, *args, **kwargs):
            try:
                fun(self, multiarg, *args, **kwargs)
            except BaseException as x:
                print '[%s]  %s' % (multiarg, x)
                errors.append(x)

        threads = [ threading.Thread(target=tf, args=(self, errors, x) + args, kwargs=kwargs) for x in multiarg ]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

        if errors:
            raise errors[0]

    return xxx


def log(fun):
    '''
    The logging decorator
    '''
    def xxx(*args, **kwargs):
        if args:
            log_str = '( %s :: %s )' % (args[0].__class__.__name__, fun.func_name)
        else:
            log_str = '%s' % fun.func_name

        print '> started %s' % log_str
        ret = fun(*args, **kwargs)
        print '> finished %s' % log_str

        return ret
    return xxx


def group2hosts(group):
    url = 'https://c.yandex-team.ru/api/groups2hosts/' + group
    requests.packages.urllib3.disable_warnings()
    r = requests.get(url)
    if r.status_code != 200:
        raise RuntimeError('Cannot fetch ' + url)
    data = r.text
    return [x.strip() for x in data.strip().split('\n')]
