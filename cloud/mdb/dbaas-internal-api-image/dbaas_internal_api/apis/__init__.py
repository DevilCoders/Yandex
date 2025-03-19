# -*- coding: utf-8 -*-
"""
DBaaS Internal API REST Package
"""

from ..core import Api, json_body_middleware, log_middleware, read_only_middleware, stat_middleware, tracing_middleware
from ..dbs.postgresql import metadb_middleware

API = Api(
    catch_all_404s=True,
    decorators=[
        log_middleware,
        tracing_middleware,
        stat_middleware,
        json_body_middleware,
        read_only_middleware,
        metadb_middleware,
    ],
)


def init_apis(app):
    """
    Initialize apis with app
    """
    # pylint: disable=unused-variable,too-many-locals,import-outside-toplevel,unused-import
    from . import support  # noqa
    from . import slb  # noqa
    from . import stat  # noqa
    from . import config  # noqa
    from . import info  # noqa
    from . import console  # noqa
    from . import quota  # noqa
    from . import resource_presets  # noqa
    from . import operations  # noqa
    from . import create  # noqa
    from . import restore  # noqa
    from . import delete  # noqa
    from . import modify  # noqa
    from . import dynamic_handlers  # noqa
    from . import backups  # noqa
    from . import start  # noqa
    from . import stop  # noqa
    from . import move  # noqa
    from . import search  # noqa
    from . import maintenance  # noqa
    from . import alerts  # noqa

    API.init_app(app)
