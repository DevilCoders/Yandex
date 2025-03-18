# coding: utf-8


class BaseManager(object):
    default_block_timeout = 0

    def __init__(self, hosts=None, prefix=None, **kwargs):
        self.hosts = hosts or ['localhost']
        self.prefix = prefix or ''

    def close(self):
        pass

    def get_hosts(self):
        hosts = self.hosts

        if not isinstance(hosts, (list, tuple)):
            hosts = [hosts]

        return hosts

    def get_full_name(self, name, separator=':'):
        def join(value):
            if isinstance(value, (tuple, list)):
                return separator.join(value)
            return value

        head = join(self.prefix)
        tail = join(name)

        if head:
            return join([head, tail])
        else:
            return tail

    def lock(self, name):
        """Возвращает объект лока с интерфейсом BaseLock
        для заданного имени
        """
        raise NotImplementedError

    def lock_from_context(self, context):
        """Возвращает лок, восстановленный из контекста
        """
        raise NotImplementedError


class BaseLock(object):
    def __init__(self, manager, name, timeout, block, block_timeout):
        self.manager = manager
        self.name = name
        self.timeout = timeout
        self.block = block
        self.block_timeout = block_timeout

        self.is_locked = False

    def acquire(self):
        """Берет лок. Возврщает True если удалось
        """
        raise NotImplementedError

    def release(self):
        """Отпускает лок. Возвращает True если удалось
        """
        raise NotImplementedError

    def get_context(self):
        """Возвращает контекст из которго можно потом востановить лок.
        Должен пиклиться.
        """
        raise NotImplementedError

    def check_acquired(self):
        """Проверяет, занят ли лок, не захватывая его.
        """
        raise NotImplementedError

    def __enter__(self):
        self.is_locked = self.acquire()

        return self.is_locked

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.is_locked:
            self.release()
