import core.db
import resources
import pprint

def main(db, master):
    sum_sq = 0
    over_hosts = 0
    hosts = list(resources.gencfg_hosts(master))
    for host in hosts:
        if host.power < 0:
            sum_sq += host.power * host.power
            over_hosts += 1

    print min(hosts, key=lambda host: host.power)

    print 'OVERCOMMIT REPORT'
    print '\t sum of squares:', sum_sq
    print '\t host count:', over_hosts

if __name__ == '__main__':
    main(core.db.CURDB, 'SAS_WEB_BASE')
