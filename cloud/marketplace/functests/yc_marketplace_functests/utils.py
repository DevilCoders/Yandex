import json

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.db.models import DB_LIST_BY_NAME
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from yc_common.clients.kikimr.sql import render_value

# from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id

BA_ID = "a6q00000234567843241"
PUBLISHER_ACCOUNT_ID = "a6q00000234567843242"
PRODUCT_ID = "d7600000234567843241"
FAMILY_ID = "d7699999234567843241"
FORM_ID = "d7601110234567843241"
MB = 1024 ** 2
GB = 1024 ** 3


def _create_product_family(client: MarketplaceClient, generate_id, name, os_product_id, skus=None, form_id=FORM_ID,
                           meta=None):
    return client.create_os_product_family(
        billing_account_id=BA_ID,
        name=name,
        image_id=generate_id(),
        pricing_options=OsProductFamily.PricingOptions.FREE,
        resource_spec=ResourceSpec({
            "memory": 4 * MB,
            "cores": 4,
            "disk_size": 30 * GB,
            "user_data_form_id": "linux",
        }),
        os_product_id=os_product_id,
        skus=skus,
        form_id=form_id,
        meta=meta,
    )


def _create_product(client: MarketplaceClient, name, category_ids=None, logo_id=None, vendor=None, meta=None,
                    slug=None):
    return client.create_os_product(
        billing_account_id=BA_ID,
        name=name,
        labels={"foo": "bar"},
        description="Some test description",
        logo_id=logo_id,
        category_ids=category_ids,
        vendor=vendor,
        meta=meta,
        slug=slug,
    )


def _create_saas_product(client: MarketplaceClient, name, category_ids=None, logo_id=None, eula_id=None, vendor=None,
                         meta=None,
                         slug=None):
    return client.create_saas_product(
        billing_account_id=BA_ID,
        name=name,
        labels={"foo": "bar"},
        description="Some test description",
        logo_id=logo_id,
        eula_id=eula_id,
        category_ids=category_ids,
        vendor=vendor,
        meta=meta,
        slug=slug,
    )


def _create_simple_product(client: MarketplaceClient, name, category_ids=None, logo_id=None, eula_id=None, vendor=None,
                           meta=None,
                           slug=None):
    return client.create_simple_product(
        billing_account_id=BA_ID,
        name=name,
        labels={"foo": "bar"},
        description="Some test description",
        logo_id=logo_id,
        eula_id=eula_id,
        category_ids=category_ids,
        vendor=vendor,
        meta=meta,
        slug=slug,
    )


def render(data):
    if isinstance(data, (list, dict)):
        return render_value(json.dumps(data))
    else:
        return render_value(data)


class DBFixture(object):
    template = "INSERT INTO {table} ({columns}) VALUES {values}"

    def __init__(self, data):
        self.data = []
        for table_name in data:
            columns = []
            values = []
            for row in data[table_name]:
                if not len(columns):
                    columns = row.keys()

                values.append("({})".format(",".join(render(row[key]) for key in columns)))

            self.data.append({
                "table": table_name,
                "columns": ",".join(columns),
                "values": ",".join(values),
            })

    def render_apply(self):
        res = []
        for table in self.data:
            if len(table["values"]):
                res.append(self.template.format(**table))

        return "; ".join(res)

    @staticmethod
    def render_clean():
        return "; ".join("DELETE FROM {}".format(table_name) for table_name in DB_LIST_BY_NAME)
