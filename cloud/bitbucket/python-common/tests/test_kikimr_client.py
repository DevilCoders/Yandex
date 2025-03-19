from yc_common.clients.kikimr import KikimrTableSpec, KikimrTable, KikimrDataType as kdt


def test_kikimr_table():
    test_table = KikimrTable(
        database_id="fake_db",
        table_name="test_table",
        table_spec=KikimrTableSpec(
            columns={
                "id": kdt.UINT64,
                "Decimal": kdt.DECIMAL,
            },
            primary_keys=["id"]
        )
    )

    assert test_table.initial_spec().columns == {"decimal": "Decimal(22,9)", "id": "Uint64"}
