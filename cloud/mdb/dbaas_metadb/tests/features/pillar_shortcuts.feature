Feature: Pillar helpers
  Background:
    Given default database
    And cloud with default quota
    And folder
    And "flavor1" flavor

  Scenario: Convert salt path to json path
    When I execute query
    """
    SELECT code.salt2json_path('{a,b,c}')::text
    """
    Then it returns
    """
    - ['{a,b,c}']
    """
    When I execute query
    """
    SELECT code.salt2json_path('a:b:c')::text
    """
    Then it returns
    """
    - ['{a,b,c}']
    """
    When I execute query
    """
    SELECT code.salt2json_path('a')::text
    """
    Then it returns
    """
    - ['{a}']
    """

  Scenario: Pillar shortcuts
    Given new fqdn
    And cluster
    And subcluster
    And shard
    And host
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"foo": {"bar": 1, "baz": 2}, "key1": {"key2": 1}}'
    )
    """
    Then it success
    When I execute query
    """
    SELECT code.easy_update_pillar(:cid, '{key1,key2}', '42'::jsonb)
    """
    Then it success
    When I execute query
    """
    SELECT value->'key1'->'key2' AS val
      FROM dbaas.pillar
     WHERE cid = :cid
    """
    Then it returns
    """
    - [42]
    """
    When I execute query
    """
    SELECT code.easy_delete_pillar(:cid, '{foo,bar}')
    """
    Then it success
    When I execute query
    """
    SELECT value->'foo' AS val
      FROM dbaas.pillar
     WHERE cid = :cid
    """
    Then it returns
    """
    - [{"baz": 2}]
    """
    When I execute query
    """
    SELECT code.easy_get_pillar(:cid, '{key1,key2}')
    """
    Then it returns
    """
    - [42]
    """
    When I execute query
    """
    SELECT code.easy_get_pillar(:cid) AS val
    """
    Then it returns
    """
    - [{"foo": {"baz": 2}, "key1": {"key2": 42}}]
    """
