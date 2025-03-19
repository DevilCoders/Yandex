import logging
import re
import socket

log = logging.getLogger(__name__)

def mongodbInfo():
    fqdn = socket.getfqdn()
    replSet = None
    isHidden = None
    isArbiter = None
    log.debug("Run mongodb custom grains")

    m = re.search('diskdb(-hidden|-arbiter)?(\d+)\w\.disk\.yandex\.net', fqdn)
    if m:
        replSet = 'disk' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('syncdb(-hidden|-arbiter)?(\d+)\w', fqdn)
    if m:
        replSet = 'sync' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('unit(-hidden|-arbiter)?(\d+)', fqdn)
    if m:
        replSet = 'disk-unit-' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('smartdb(-hidden|-arbiter)?(\d+)', fqdn)
    if m:
        replSet = 'disk-smart-' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('hiddb(-hidden|-arbiter)?(\d+)\w', fqdn)
    if m:
        replSet = 'disk-mongodb-hid-' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('sysdb(-hidden|-arbiter)?(\d+)\w\.disk', fqdn)
    if m:
        # we're not planning to create more than one system replica set
        replSet = 'disk-mpfs-sys'
        log.debug("Using '{0}' as replSet".format(replSet))
    m = re.search('auxdb(-hidden|-arbiter)?(\d+)', fqdn)
    if m:
        replSet = 'disk-aux-' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))

    m = re.search('diskdb(-hidden|-arbiter)?(\d+)\w.[dt]st', fqdn)
    if m:
        replSet = 'disk' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))

    m = re.search('userdb(-hidden|-arbiter)?(\d+)', fqdn)
    if m:
        replSet = 'disk_test_mongodb-unit' + str(int(m.group(2)))
        log.debug("Using '{0}' as replSet".format(replSet))

    m = re.search('sysdb(-hidden|-arbiter)?(\d+)\w\.dst', fqdn)
    if m:
        replSet = 'disk_test_mongodb-sys'
        log.debug("Using '{0}' as replSet".format(replSet))

    m = re.search('miscdb(-hidden|-arbiter)?(\d+)', fqdn)
    if m:
        replSet = 'disk-misc-' + m.group(2)
        log.debug("Using '{0}' as replSet".format(replSet))

    m = re.search('hidden', fqdn)
    isHidden = (m != None)
    m = re.search('arbiter', fqdn)
    isArbiter = (m != None)

    return {'mongodb': {
        'replSet': replSet,
        'isHidden': isHidden,
        'isArbiter': isArbiter
        }}


