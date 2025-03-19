from unittest.mock import MagicMock

import pytest

import cloud.marketplace.common.yc_marketplace_common as yc_marketplace_common
from yc_common.clients.kikimr.client import _KikimrTxConnection
from cloud.marketplace.common.yc_marketplace_common.utils.errors import ReadOnlyError
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


def test_mkt_transaction_with_out_tx(mocker):
    mocker.patch("yc_common.config.get_value", return_value=False)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.transactions.marketplace_db")
    mocker.patch("yc_common.clients.kikimr.client._KikimrTxConnection")

    @mkt_transaction()
    def mock_function(tx):
        assert isinstance(tx, MagicMock)

    mock_function()

    # assert yc_common.clients.kikimr.client._KikimrTxConnection.call_count == 1 Иногда не работает, надо разобраться
    assert yc_marketplace_common.utils.transactions.marketplace_db.call_count == 1


def test_mkt_transaction_with_tx(mocker):
    mocker.patch("yc_common.config.get_value", return_value=False)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.transactions.marketplace_db")
    mocker.patch("yc_common.clients.kikimr.client._KikimrTxConnection")
    explicit_tx = MagicMock(_KikimrTxConnection)

    @mkt_transaction()
    def mock_function(tx):
        assert explicit_tx == tx

    mock_function(tx=explicit_tx)

    assert explicit_tx.commit.call_count == 0


def test_tx_for_ro(mocker):
    mocker.patch("yc_common.config.get_value", return_value=True)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.transactions.marketplace_db")
    mocker.patch("yc_common.clients.kikimr.client._KikimrTxConnection")

    with pytest.raises(ReadOnlyError):
        @mkt_transaction()
        def mock_function(tx):
            pass

        mock_function()
