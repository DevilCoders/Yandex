from cocaine.services import Service
from tornado import gen
from tornado import ioloop
import os

endpoints_old = [("localhost",10053)]
endpoints_new = [("cocs10e.ape.yandex.net",10053)]
indexes = [
        ["groups", ["group", "active"]],
        ["manifests", ["app"]],
        ["apps", ["app"]],
        ["profiles", ["profile"]],
        ["runlists", ["runlist"]]
]

@gen.coroutine
def main():
    old_cocaine = os.system("dpkg -l | grep 'ii  libcocaine-core3' | awk '{print $3}' | grep 0.12.9 > /dev/null")
    if old_cocaine != 0:
        print "found new cocaine. skipping..."
        raise gen.Return(0)

    unicorn = Service("unicorn", endpoints = endpoints_old)
    storage_old = Service("storage", endpoints = endpoints_old)
    storage_new = Service("storage", endpoints = endpoints_new)

    try:
        chan_unicorn = yield unicorn.lock("/sync_indexes")
        result = yield chan_unicorn.rx.get()
    except:
        print "2;service unicorn fail"
        gen.Return(1)

    yield gen.sleep(10)

    for (collection, tags) in indexes:
        try:
            old_channel = yield storage_old.find(collection, tags)
            old_indexes = yield old_channel.rx.get()
        except:
            print "local locator failed."
            raise gen.Return(1)
        try:
            new_channel = yield storage_new.find(collection, tags)
            new_indexes = yield new_channel.rx.get()
        except:
            print "new locator " + str(endpoints_new) + " failed!" 
            raise gen.Return(1)

        for new_key in new_indexes:
            if new_key not in old_indexes:
                try:
                    print str(new_key) + " not found in old indexes in collection: " + str(collection) + ". Create it!"
                    new_channel = yield storage_new.read(collection, new_key)
                    value_msgpack = yield new_channel.rx.get()
                    old_channel = yield storage_old.write(collection, new_key, value_msgpack, tags)
                    yield old_channel.rx.get()
                except:
                    print "Create new index in old indexes failed. key name: " + str(new_key)
                    continue

        for old_key in old_indexes:
            if old_key not in new_indexes:
                try:
                    print str(old_key) + " not found in new indexes in collection: " + str(collection) + ". Create it!"
                    old_channel = yield storage_old.read(collection, old_key)
                    value_msgpack = yield old_channel.rx.get()
                    new_channel = yield storage_new.write(collection, old_key, value_msgpack, tags)
                    yield new_channel.rx.get()
                except:
                    print "Create new index in new indexes failed. key name: " + str(old_key)
                    continue

    yield chan_unicorn.tx.close()

try:
    ioloop.IOLoop.current().run_sync(main, timeout=1500)
except ioloop.TimeoutError:
    print "Timeout"
    exit(1)

