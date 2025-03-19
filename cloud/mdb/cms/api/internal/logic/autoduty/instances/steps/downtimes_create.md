# create downtimes

At first run CMS creates downtime for a single fqdn using Juggler API and stores downtime_id in the state.\
Every next run it takes downtime_id from state and prolongs it up to 1 hour from current time.
