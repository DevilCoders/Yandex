from newcachelib import CacheManager
import time

cache_manager = CacheManager.create_from_sentinels_string(
    ','.join([
        'man1-8872.search.yandex.net:21750',
        'myt1-2531.search.yandex.net:21750',
        'sas1-3116.search.yandex.net:21750',
    ]),
    redis_db=0
)


@cache_manager.memoize(key_prefix="q", timeout=10)
def q(x, y=42, *args, **kwargs):
    return "HEY HEY {x}, {y}, {args} + {kwargs}".format(x=x, y=y, args=args, kwargs=kwargs)


@cache_manager.memoize(key_prefix="g", timeout=10)
def g(x):
    return "Hello, {}".format(x)


@cache_manager.memoize(key_prefix="h", timeout=10)
def h(x):
    return " and, {}".format(x)


@cache_manager.memoize(key_prefix="f", timeout=10)
def f(x):
    return g(x + 1) + h(x * 3)


@cache_manager.memoize(key_prefix="factorial", timeout=10)
def factorial(x=10):
    if x <= 1:
        return 1
    return factorial(x - 1) * x


def main():
    print time.time(), "f(3) = ", f(3)
    print time.time(), "f(3) cache time = ", f.cached_timestamp(3)
    print time.time(), "f(3) = ", f(3)
    print time.time(), "f(3) = ", f(3)
    print time.time(), "f(3) = ", f(3)
    print time.time(), "f(42) = ", f(42)
    print time.time(), "f(42) = ", f(42)
    print time.time(), "all fs", f.get_all_cached_single_arg()
    print time.time(), "all gs", g.get_all_cached_single_arg()
    print time.time(), "all hs", h.get_all_cached_single_arg()

    print time.time(), "factorial(10) = ", factorial(10)
    print time.time(), "all factorials", factorial.get_all_cached_single_arg()
    print time.time(), "all fs", f.get_all_cached_single_arg()

    print time.time(), "q(13) = ", q(13)
    print time.time(), "q(43) = ", q(13)
    print time.time(), "q(x=142, y=34) = ", q(x=142, y=34)
    print time.time(), "q(**{'x': 16, 'y': 17}) = ", q(**{'x': 16, 'y': 17})
    print time.time(), "q(13, z=99) = ", q(13, z=99)
    print time.time(), "q(13, 14, 15, 16, z=99, a=100, b=101) = ", q(13, 14, 15, 16, z=99, a=100, b=101)
    print time.time(), "all qs", q.get_all_cached()


if __name__ == "__main__":
    main()
