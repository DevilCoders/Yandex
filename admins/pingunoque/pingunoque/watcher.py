"""pingunoque watcher thread"""
import time
import threading
import urllib2
from pingunoque import log, tcp, icmp, ipset, dnscache, config

_RELOAD_WATCHERS_EVENT = threading.Event()

class WatcherThread(threading.Thread):
    """Thread where checks made"""
    def __init__(self, *args, **kwargs):
        super(WatcherThread, self).__init__(*args, **kwargs)
        self._stop = threading.Event()
        self.new_cfg = threading.Event()
        self.cfg = None

    def stop(self):
        """Send stop for watcher thread"""
        self._stop.set()

    def reload_config(self, new_cfg):
        """set new config object and flag to indicate config changes"""
        self.cfg = new_cfg
        self.new_cfg.set()

    def wait_exit(self, timeout):
        """Wait stop event"""
        return self._stop.wait(timeout)


def host_watcher(host):
    """Body of checker thread"""

    thr = threading.current_thread()
    thr.name = ".".join(host.name.split(".")[:-2]) or host.name
    log.info("%s", thr)
    alive = True

    @_retry(host.cfg.check_retry)
    def check(host):
        "Helper for pick appropriate check type"
        if host.cfg.check_type == 'tcp':
            return tcp.check(host)
        elif host.cfg.check_type == 'icmp':
            return icmp.check(host)
        else:
            raise RuntimeError("Unknown check type")

    while True:
        try:
            result = check(host)

            if alive and not result:
                log.info("becomes unreachable")
                alive = False
                ipset.ban(host)
            elif not alive and result:
                log.info("becomes alive")
                alive = True
                ipset.unban(host)
        except dnscache.DNSCacheNXDomain:
            # if we couldn't resolve address due to dns error NXDOMAIN,
            # we should remove host's ips from ipset list and reload watchers
            log.warn("Got NXDOMAIN, reload watchers")
            ipset.unban(host, stale_cache=True)
            _RELOAD_WATCHERS_EVENT.set()
        except dnscache.DNSCacheGAIError as exc:
            log.debug("DNS Error: %s", exc)
            if host.dns_errors > host.cfg.dns_error_limit and host.cfg.unban_on_dns_errors:
                ipset.unban(host, stale_cache=True)
                log.warn("Got max dns errors(%s), unban %s and reload watchers",
                         host.dns_errors, host)
                _RELOAD_WATCHERS_EVENT.set()
            # if dns error is not NXDOMAIN, revert host alive status
            # got to next iteration for re-add/re-remove it to/from ipset list
            alive = not alive

        if thr.new_cfg.is_set():
            log.info("Reload config")
            host.cfg = thr.cfg
            thr.new_cfg.clear()

        if thr.wait_exit(host.cfg.check_period):
            log.info("Got exit event, exiting")
            ipset.unban(host)
            return

def _retry(num):
    "Helper decorator, retry a check if errors"
    def decorator(fun):
        "decorator"
        def wrapper(*args):
            "wrapper"
            for _ in xrange(num):
                if fun(*args):
                    return True
            log.trace("Retry count '%d' exhausted: '%s'", num, fun)
            return False
        return wrapper
    return decorator


class WatchersPool(object):
    """
    Watchers pool
    TODO: add locks
    """
    def __init__(self):
        self.__pool = []
        self.__initialized = False

    def present(self):
        """check pool"""
        return bool(self.__pool)

    def reload(self):
        """
        TODO: send exitSignal to all threads, re-read config,
        get new hosts from Conductor, create new threads
        """
        if self.__pool:
            if not _RELOAD_WATCHERS_EVENT.wait(0.1):
                return
            _RELOAD_WATCHERS_EVENT.clear()

        log.info("Loading config")
        cfg = config.load_and_validate(log)
        # pylint: disable=no-member
        log.info("Watching on %s with %s", cfg.watched_groups, cfg.check_type)
        log.info("Check port=%s every %s sec", cfg.check_port, cfg.check_period)
        # pylint: enable=no-member

        if self.__initialized and not self.__pool:
            duration = cfg.dns_cache_time # pylint: disable=no-member
            log.error("Empty watchers list, reload after dns_cache_time=%s", duration)
            time.sleep(duration)

        log.info("Reloading hosts")
        hosts = WatchersPool.load_hosts(cfg)

        log.info("Reloading watchers")
        new_watcher_threads = []
        if self.__pool:
            for (thread, hostname) in self.__pool:
                if hostname not in hosts:
                    log.info("Removing from watch")
                    thread.stop()
                else:
                    log.info("Keeping in watch")
                    thread.reload_config(cfg)
                    new_watcher_threads.append((thread, hostname))
                    del hosts[hostname]
        for hostname, info in hosts.items():
            thread = WatcherThread(target=host_watcher, args=(info,))
            thread.start()
            new_watcher_threads.append((thread, hostname))
        self.__pool = new_watcher_threads
        self.__initialized = True


    @staticmethod
    def load_hosts(cfg):
        """
        Expand conductor groups2hosts and return set of hosts
        """
        hosts = {}
        api = "https://c.yandex-team.ru/api-cached/groups2hosts"
        for group in cfg.watched_groups:
            try:
                for hname in urllib2.urlopen("{0}/{1}".format(api, group)):
                    hname = hname.strip()
                    try:
                        if hname not in hosts:
                            hosts[hname] = dnscache.DNSCacher(hname, cfg)
                    except dnscache.DNSCacheGAIError as exc:
                        log.error("Couldn't resolve host %r: %s", hname, exc)
            except (urllib2.URLError, urllib2.HTTPError) as exc:
                log.exception("Failed to get host list for '%s'", group)
        return hosts


    def stop(self):
        """stop all threads in the pool"""
        log.info("Waiting child threads to terminate")
        for (w_thread, _) in self.__pool:
            w_thread.stop()
            w_thread.join()

    @staticmethod
    def notify_reload(_sig=None, _frame=None):
        """notify pool about reload event"""
        _RELOAD_WATCHERS_EVENT.set()
