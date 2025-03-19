# Decide if letting dom0 would affect cluster health.
If OK to let go, this step prints short "ok" with some minor notes if any.
If NOT ok: step groups fqdns in problem groups:

1. `outdated info` - information from health is stale, cannot make a decision
1. `knows nothing about` - health does not know about them.
    This happens to non-db containers etc.
1. `would degrade now` - letting them go to Wall-e would make cluster degrade.
    This logic is db-specific.
1. `looking suspicious` - 100% sure to let go because of their configuration, but they do not satisfy common "safe-to-let-go" condition.
    Sample reason to get here: to be in "prod" env and be one-legged cluster.

Single container of dom0 with problems looks like this: `'sas-joljy8l75wxw6l4t.db.yandex.net' of 'mdbi0c0pnv8830oprruh'. HA: true (сluster+shard), plays roles: redis_cluster, 1.4 GB, other healthy nodes left: 1 of 3 nodes total, env: 'prod'`.

* `HA: true (сluster+shard)` shows if it's a Highly Available cluster (and/or shard) or not
* `same healthy roles: 1 of 1` shows you that M other nodes with same role are healthy of N nodes in total
* `Roles: postgresql_cluster` is role which current fqnd plays
* `1.4 GB` is space given for container
* `mdb4vengui87m649t5k5` is name of cluster
* `sas-xndgjoqe7crhan84.db.yandex.net` is fqdn of container

Typical full answer could look like this:

    |  5 health of clusters: knows nothing about 2:
    |                        'dbaas-worker-test01h.db.yandex.net' of 'dbaas-worker-test01'
    |                        'mlockdb01h.db.yandex.net' of 'mlockdb01'
    |                        1 would degrade now:
    |                        'sas-joljy8l75wxw6l4t.db.yandex.net' of 'mdbi0c0pnv8830oprruh'. HA: true (сluster), roles: redis_cluster, 1.2 GB, other healthy nodes left: 1 of 3 nodes total, env: 'prod'
    |                        and 1 looking suspicious but I will ignore them:
    |                        'sas-ab5hhzcrlvmkwg8o.db.yandex.net' of 'mdbm333jr41532k23chr'. HA: false, roles: postgresql_cluster, 1.3 GB, other healthy nodes left: 0 of 1 nodes total

The happy path answer could look like this:

    health of clusters: 3 ok

## What to do when you see this?
If you see these messages, CMS thinks that listed clusters are NOT OK to let go. So the correct way:

1. For DB clusters:
    1. You can move containers to other dom0s, and CMS will see that problem is solved and will continue it's work. To see the size of container - https://st.yandex-team.ru/MDB-9704, to automatically move container - https://st.yandex-team.ru/MDB-9177.
    1. if think that CMS could probably ALLOW specific cluster, you SHOULD ask specific database team, providing them with CMS's output. For example see https://st.yandex-team.ru/MDB-9193. You should make a PR or a ticket.
    1. If moving and resetup is impossible, the last thing you can do - get approve from DB-team and let go dom0 with degraded availability
1. For well known container groups, specified in config, CMS looks for health in Juggler.
1. For other containers
    1. find a responsible, make a ticket and automate it.
