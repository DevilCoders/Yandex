import mimetypes
import tempfile

import requests
from yc_common import config
from yc_marketplace_migrations.migration import BaseMigration

from cloud.marketplace.common.yc_marketplace_common.db.models import avatars_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import publishers_table
from cloud.marketplace.common.yc_marketplace_common.lib import Publisher
from cloud.marketplace.common.yc_marketplace_common.lib.avatar import Avatar
from cloud.marketplace.common.yc_marketplace_common.lib.avatar import check_picture_mimetype
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MigrationNoRollback


def get_avatar_host():
    return config.get_value("marketplace.mds.host",
                            default="https://avatars.mdst.yandex.net")


def get_url(ava, size: str) -> str:
    meta = ava["meta"].get("sizes")
    if size in meta:
        return get_avatar_host() + ava["meta"]["sizes"][size]["path"]


class Migration(BaseMigration):
    def execute(self) -> None:
        logo_iter = publishers_table.client.select("SELECT logo_uri, logo_id, folder_id, id FROM $table")
        for row in logo_iter:
            with marketplace_db().with_table_scope(avatars_table).transaction() as tx:
                ava = Avatar._get_image(tx, row["logo_uri"])

            with tempfile.TemporaryFile() as data:
                data.write(requests.get(get_url(ava, "orig")).content)
                data.seek(0)
                filename = "avatar." + ava["meta"]["meta"]["orig-format"].lower()
                mimetype, _ = mimetypes.guess_type(filename)
                check_picture_mimetype(mimetype)
                new_ava = Avatar.create(filename, row["folder_id"], data)
                Publisher.rpc_update(PublisherUpdateRequest({"publisherId": row["id"], "logoId": new_ava.id}))

    def rollback(self):
        raise MigrationNoRollback
