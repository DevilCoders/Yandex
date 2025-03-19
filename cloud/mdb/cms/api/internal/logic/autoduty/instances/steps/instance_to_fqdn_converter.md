# convert instance ID to FQDN

At first run CMS tries to resolve instance ID to FQDN using metadb.\
If there is no data in metadb it considers that FQDN == InstanceID.

Every next run CMS takes FQDN from the state.
