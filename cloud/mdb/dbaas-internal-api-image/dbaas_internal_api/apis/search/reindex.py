# -*- coding: utf-8 -*-
"""
DBaaS Internal API reindex apis
"""

from datetime import datetime
from typing import List, Dict

from dateutil.tz import tzutc
from flask.views import MethodView

from .. import API, parse_kwargs
from ...core.auth import check_auth
from ...utils import metadb, search_renders, logs
from ...utils.register import get_supported_clusters
from ...utils.types import ClusterInfo, ClusterVisibility
from ..schemas.search import ReindexCloudRequestSchema


def _now() -> datetime:
    return datetime.now(tzutc())


def _get_changed_at(cids: List[str]) -> Dict[str, datetime]:
    ret = {}
    for cd in metadb.get_clusters_change_dates(cids):
        ret[cd['cid']] = cd['changed_at']
    return ret


@API.resource('/mdb/<version>/search/cloud/<string:cloud_id>:reindex')
class ReindexCloudV1(MethodView):
    """Reindex all clusters in cloud"""

    @check_auth(
        explicit_action='mdb.all.read',
        entity_type='cloud',
    )
    @parse_kwargs.with_schema(ReindexCloudRequestSchema)
    def post(
        self,
        cloud_id: str,
        include_deleted: bool,
        reindex_timestamp: datetime = None,
        version: str = None,  # pylint: disable=unused-argument
    ) -> dict:
        """Reindex all clusters in cloud"""
        if reindex_timestamp is None:
            reindex_timestamp = _now()
        visibility = ClusterVisibility.all if include_deleted else ClusterVisibility.visible
        supported_clusters = get_supported_clusters()
        with metadb.commit_on_success():
            reindex_docs = []
            for folder in metadb.get_folders_by_cloud(cloud_ext_id=cloud_id):
                clusters_in_folder = [
                    ClusterInfo.make(cd)
                    for cd in metadb.get_clusters_by_folder(
                        limit=None, folder_id=folder['folder_id'], visibility=visibility
                    )
                    if cd['type'] in supported_clusters
                ]
                changed_at = _get_changed_at([c.cid for c in clusters_in_folder])
                logs.log_warn("changed_at %r", changed_at)
                for cluster in clusters_in_folder:
                    if cluster.cid not in changed_at:
                        logs.log_warn(
                            "strange cluster %r doesn't have public tasks. "
                            "We can't reindex its cause can't define the timestamp of cluster change.",
                            cluster.cid,
                        )
                        continue

                    doc = search_renders.make_doc(
                        cluster=cluster,
                        timestamp=changed_at[cluster.cid],
                        folder_ext_id=folder['folder_ext_id'],
                        cloud_ext_id=cloud_id,
                        reindex_timestamp=reindex_timestamp,
                    )
                    reindex_docs.append(doc)
            metadb.add_to_search_queue(reindex_docs)
            return {'docs': reindex_docs}
