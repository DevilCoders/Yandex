OWNER(g:cloud-marketplace)

PY3_LIBRARY(yc_marketplace_migrations)

PEERDIR(
	library/python/resource
)

PY_SRCS(
	NAMESPACE yc_marketplace_migrations
	__init__.py
	utils/__init__.py
	utils/helpers.py
	utils/errors.py
	table.py
	migration.py
	base.py
)

END()
