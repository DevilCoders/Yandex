-- NOTE: partly filled relations for test purposes only!

CREATE TABLE `test/mkt/meta/product_versions` (
    id Utf8,
    name Utf8,

    product_id Utf8,
    publisher_id Utf8,
    state  Utf8,

    payload Json,
    license_rules Json,

    created_at Datetime,
    update_at Datetime,

    PRIMARY KEY (id)
);
