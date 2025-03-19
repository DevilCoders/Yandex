# cli scripts for CMS

CMS has no UI. So we use shell scripts. Switch to this directory in your local copy of arcadia.

## List requests

`./list.sh`.
When **no requests**, you will see

![no requests](https://jing.yandex-team.ru/files/chapson/2020-04-12_18-38-45.png)

When **1 request** available, you will see https://paste.yandex-team.ru/1027719/text

![1 requests](https://jing.yandex-team.ru/files/chapson/2020-05-18_13-41-49.png)

Autoduty has already made a decision. He says "escalated", so the request must be handled by duty. In "cms expl" you can see why this decision has been made.

You can also list hosts we let go and Wall-e still didn't return to us in section "AWAITING BACK FROM WALL-E":
![2 hosts yet to be returned](https://jing.yandex-team.ru/files/chapson/2020-07-28_13-26-45.png)

## Handle requests

Please, check [wiki table](https://wiki.yandex-team.ru/MDB/internal/teams/core/Development/cms/#obrabotkazajavokotwall-e) first, to understand the algorighm. And after you handled the request, write it to the wiki, so that we could make this handling automatic.

You have 2 options: approve Wall-e's request to perform the operation requests, or reject. You will need to know master database fqdn.
Find the database hosts in salt scripts, probably in [pillar/top.sls](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/salt/pillar/top.sls?rev=6660831#L474-479). You can learn it by SSH-ing on host from db, then `psql` and executing `SELECT pg_is_in_recovery()`, you will get
* FALSE for master
* TRUE for slave

### Approve
`cms_req_id="" ./approve.sh`

Successful:
![success](https://jing.yandex-team.ru/files/chapson/2020-04-12_20-12-04.png)

### Reject

Note: you should write explicit explanation.

`cms_req_id="" cms_explanation="no you do not" ./reject.sh`

Successful:
![success](https://jing.yandex-team.ru/files/chapson/2020-04-12_20-39-27.png)

## Note:
You can add `cms_master_db="cms-db-test01h.db.yandex.net"` to any script to override used database.
