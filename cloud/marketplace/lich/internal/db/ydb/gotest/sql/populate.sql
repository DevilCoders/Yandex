REPLACE INTO `test/mkt/meta/product_versions`
    (
    id, name, product_id, publisher_id, state, payload, license_rules)
VALUES
    ("id1", "prod_version1", "product1", "publisher1",  "active", '{"foo1": "bar1"}', '[1,2,3]' ),
    ("id2", "prod_version1", "product2", "publisher2",  "broken", '{"foo2": "bar2"}', '[4,5,6]' ),
    ("id3", "prod_version2", "product1", "publisher1",  "active", '{"zoo": "foo"}', '{"any": "object"}'),
    ("id4", "prod_version2", "product2", "publisher2",  "active", NULL, '{"any": "object"}'),
    ("id5", "prod_version2", "product3", "publisher2",  "active", '{"zoo": "foo"}', NULL);
