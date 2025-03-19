# health of cluster

Goes to mdb-health service, asks neighbours info.\
If there are enough alive neighbours CMS allows to continue.\
Otherwise it waits.\
If there is outdated info it waits.\

If everything is OK CMS remembers it in the state.\
Every next run CMS will not do mdb-health request.
