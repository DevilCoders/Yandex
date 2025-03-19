# whip master

At first run CMS requests host health status from mdb-health.\
If host is not part of HA cluster or shard, CMS doesn't care about host leadership.\
If host is not master CMS remembers it in the state. Every next run CMS will not provide ant checks.\
Otherwise it creates `ensure_no_primary.sh` shipment and stores shipment id in the state.\
Every next run CMS will wait until shipment is done.
