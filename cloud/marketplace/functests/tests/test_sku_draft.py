import pytest

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import CreateSkuDraftRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftStatus
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftUnit
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def make_valid_data(generate_id):
    return {
        "name": "tariff2019",
        "description": "Simple Tariff",
        "unit": SkuDraftUnit.CORE_PER_SECOND,

        "billing_account_id": BA_ID,
        "publisher_account_id": generate_id(),
        "meta": {
            "ru": {
                "name": "Тариф 2019",
            },
            "en": {
                "name": "Tariff 2019",
            }
        },
        "pricing_versions": [{
            "pricing_expression": {
                "rates": [
                    {
                        "unit_price": "34",
                        "start_pricing_quantity": "0"
                    }
                ],
            }
        }],
    }


def test_sku_draft_crud(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    data = make_valid_data(generate_id)

    op = marketplace_client.create_sku_draft(CreateSkuDraftRequest(data))
    sd_id = op.metadata.sku_draft_id
    sd = marketplace_client.get_sku_draft(sd_id)

    assert sd.name == data["name"]

    sd_list = marketplace_client.list_sku_drafts(BA_ID)

    assert len(sd_list.sku_drafts) == 1
    assert sd_list.sku_drafts[0].name == data["name"]


def test_ba_id_filtration(marketplace_client: MarketplaceClient, marketplace_private_client: MarketplacePrivateClient,
                          generate_id, db_fixture):
    data = make_valid_data(generate_id)

    op = marketplace_client.create_sku_draft(CreateSkuDraftRequest(data))
    sd_id = op.metadata.sku_draft_id
    sd = marketplace_client.get_sku_draft(sd_id)

    assert sd.name == data["name"]

    data_2 = make_valid_data(generate_id)
    data_2["billing_account_id"] = generate_id()
    marketplace_client.create_sku_draft(CreateSkuDraftRequest(data_2))

    sd_list = marketplace_client.list_sku_drafts(BA_ID)
    print(sd_list.sku_drafts)
    assert len(sd_list.sku_drafts) == 1
    assert sd_list.sku_drafts[0].name == data["name"]
    assert sd_list.sku_drafts[0].id == sd_id

    pl = marketplace_private_client.list_sku_drafts()
    assert len(pl.sku_drafts) == 2

    p_sd = marketplace_private_client.get_sku_draft(pl.sku_drafts[1].id)
    assert p_sd.name == data["name"]


def test_request_validation(marketplace_client: MarketplaceClient, db_fixture):
    data = {
        "name": "tariff2019",
        "description": "Simple Tariff",
        "unit": SkuDraftUnit.CORE_PER_SECOND,

        "publisher_account_id": "",
        "meta": {
            "ru": {
                "name": "Тариф 2019",
            },
            "en": {
                "name": "Tariff 2019",
            }
        },
        "pricing_versions": [{
            "pricing_expression": {
                "quantum": "1",
                "rates": [
                    {
                        "unit_price": "34",
                        "start_pricing_quantity": "0"
                    }
                ],
            }
        }],

    }
    with pytest.raises(Exception):
        marketplace_client.create_sku_draft(CreateSkuDraftRequest(data))


def test_reject(marketplace_client: MarketplaceClient, marketplace_private_client: MarketplacePrivateClient,
                       generate_id, db_fixture):
    data = make_valid_data(generate_id)
    op = marketplace_client.create_sku_draft(CreateSkuDraftRequest(data))
    sd_id = op.metadata.sku_draft_id
    reject_op = marketplace_private_client.reject_sku_draft(sd_id)
    assert reject_op.response.status == SkuDraftStatus.REJECTED

    sd = marketplace_client.get_sku_draft(sd_id)
    assert sd.status == SkuDraftStatus.REJECTED
