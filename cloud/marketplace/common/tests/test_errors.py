from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidProductType
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductDuplicateError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductIdError


def test_os_product_id_error():
    err = OsProductIdError()

    assert err.http_code == 404
    assert err.code == "OsProductIdError"
    assert err.message is not None


def test_invalid_product_type():
    err = InvalidProductType()
    assert err.http_code == 400


def test_duplicate_err():
    err = OsProductDuplicateError()
    assert err.http_code == 409
