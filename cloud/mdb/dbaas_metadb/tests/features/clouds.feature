Feature: Clouds API

  Background: Default database
    Given default database
      And vars "cloud_ext_id"

  Scenario: Add cloud and get it
    When I execute query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """
    When I execute query
    """
    SELECT * FROM code.get_cloud(
        i_cloud_ext_id => :cloud_ext_id,
        i_cloud_id     => NULL
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """

  Scenario Outline: Get cloud for not existed cloud
    When I execute query
    """
    SELECT *
      FROM code.get_cloud(i_cloud_ext_id => <cloud_ext_id>, i_cloud_id => <cloud_id>)
    """
    Then it returns nothing

  Examples:
    | cloud_ext_id  | cloud_id |
    | :cloud_ext_id | NULL     |
    | NULL          | -42      |

  Scenario: Update cloud usage
    When I execute query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    And I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 12.0
    gpu_used: 4
    memory_used: 1024
    ssd_space_used: 32000
    hdd_space_used: 64000
    clusters_used: 2
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => -6.0,
            i_gpu       => -2,
            i_memory    => -512,
            i_ssd_space => -16000,
            i_hdd_space => -32000,
            i_clusters  => -1
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 6.0
    gpu_used: 2
    memory_used: 512
    ssd_space_used: 16000
    hdd_space_used: 32000
    clusters_used: 1
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => -6.0,
            i_gpu       => -2,
            i_memory    => -512,
            i_ssd_space => -16000,
            i_hdd_space => -32000,
            i_clusters  => -1
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """

  Scenario: Update cloud quota
    When I execute query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    And I execute query
    """
    SELECT *
      FROM code.update_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_delta        => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 24.0
    gpu_quota: 8
    memory_quota: 2048
    ssd_space_quota: 64000
    hdd_space_quota: 128000
    clusters_quota: 4
    cpu_used: 0.0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """

  Scenario: Update cloud usage with NULL fields
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta   => code.make_quota(
            i_cpu       => NULL,
            i_gpu       => NULL,
            i_memory    => NULL,
            i_ssd_space => NULL,
            i_hdd_space => NULL,
            i_clusters  => NULL
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """

  Scenario Outline: Update usage beyond qouta
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => <cpu>,
            i_gpu       => <gpu>,
            i_memory    => <memory>,
            i_ssd_space => <ssd_space>,
            i_hdd_space => <hdd_space>,
            i_clusters  => <clusters>
        ),
        i_x_request_id => ''
    )
    """
    Then it fail with error matches "new row for relation .clouds. violates check constraint .check_quota."

    Examples:
        | cpu  | gpu | memory | ssd_space | hdd_space | clusters |
        | 14.0 | 0   | 0      | 0         | 0         | 0        |
        | 0    | 0   | 2048   | 0         | 0         | 0        |
        | 0    | 0   | 0      | 64000     | 0         | 0        |
        | 0    | 0   | 0      | 0         | 128000    | 0        |
        | 0    | 0   | 0      | 0         | 0         | 4        |
        | 0    | 6   | 0      | 0         | 0         | 0        |


  Scenario Outline: Update usage beneath zero
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => <cpu>,
            i_gpu       => <gpu>,
            i_memory    => <memory>,
            i_ssd_space => <ssd_space>,
            i_hdd_space => <hdd_space>,
            i_clusters  => <clusters>
        ),
        i_x_request_id => ''
    )
    """
    Then it fail with error matches "new row for relation .clouds. violates check constraint .check_used_non_negative."

    Examples:
        | cpu  | gpu  | memory | ssd_space | hdd_space | clusters |
        | -1.0 | 0    | 0      | 0         | 0         | 0        |
        | 0    | 0    | -2048  | 0         | 0         | 0        |
        | 0    | 0    | 0      | -64000    | 0         | 0        |
        | 0    | 0    | 0      | 0         | -128000   | 0        |
        | 0    | 0    | 0      | 0         | 0         | -4       |
        | 0    | -1   | 0      | 0         | 0         | 0        |

    Scenario Outline: Update cloud quota beneath usage
      Given successfully executed query
      """
        SELECT *
          FROM code.add_cloud(
            i_cloud_ext_id  => :cloud_ext_id,
            i_quota         => code.make_quota(
                i_cpu       => 12.0,
                i_gpu       => 4,
                i_memory    => 1024,
                i_ssd_space => 32000,
                i_hdd_space => 64000,
                i_clusters  => 2
            ),
            i_x_request_id => ''
        )
        """
        And successfully executed query
        """
        SELECT *
          FROM code.update_cloud_usage(
            i_cloud_id     => (SELECT cloud_id FROM dbaas.clouds WHERE cloud_ext_id = :cloud_ext_id),
            i_delta        => code.make_quota(
                i_cpu       => 12.0,
                i_gpu       => 4,
                i_memory    => 1024,
                i_ssd_space => 32000,
                i_hdd_space => 64000,
                i_clusters  => 2
            ),
            i_x_request_id => ''
        )
        """
        When I execute query
        """
        SELECT *
          FROM code.update_cloud_quota(
            i_cloud_ext_id => :cloud_ext_id,
            i_delta        => code.make_quota(
                i_cpu      => <cpu>,
                i_gpu      => <gpu>,
                i_memory   => <memory>,
                i_ssd_space    => <ssd_space>,
                i_hdd_space    => <hdd_space>,
                i_clusters => <clusters>
            ),
            i_x_request_id => ''
        )
        """
        Then it fail with error matches "new row for relation .clouds. violates check constraint .check_quota."

    Examples:
        | cpu  | gpu  | memory | ssd_space | hdd_space | clusters |
        | -1.0 | 0    | 0      | 0         | 0         | 0         |
        | 0    | 0    | -2048  | 0         | 0         | 0         |
        | 0    | 0    | 0      | -64000    | 0         | 0         |
        | 0    | 0    | 0      | 0         | -128000   | 0         |
        | 0    | 0    | 0      | 0         | 0         | -4        |
        | 0    | -1   | 0      | 0         | 0         | 0         |

    Scenario: Lock cloud
      Given successfully executed query
      """
      CREATE EXTENSION IF NOT EXISTS pgrowlocks
      """
      And successfully executed query
      """
      SELECT *
        FROM code.add_cloud(
            i_cloud_ext_id  => :cloud_ext_id,
            i_quota         => code.make_quota(
                i_cpu       => 12.0,
                i_gpu       => 4,
                i_memory    => 1024,
                i_ssd_space => 32000,
                i_hdd_space => 64000,
                i_clusters  => 2
            ),
            i_x_request_id => ''
      )
      """
      When I start transaction
      And I execute in transaction
      """
      SELECT *
        FROM code.lock_cloud(
            i_cloud_id => (
                SELECT cloud_id
                  FROM code.get_cloud(
                    i_cloud_ext_id => :cloud_ext_id,
                    i_cloud_id     => NULL
                )
            )
        )
      """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 12.0
    gpu_quota: 4
    memory_quota: 1024
    ssd_space_quota: 32000
    hdd_space_quota: 64000
    clusters_quota: 2
    cpu_used: 0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """
    When I execute query
    """
    SELECT modes
      FROM pgrowlocks('dbaas.clouds')
     WHERE locked_row = (
        SELECT ctid
          FROM dbaas.clouds
         WHERE cloud_ext_id = :cloud_ext_id
    )
    """
    Then it returns one row matches
    """
    modes: ['For Update']
    """
    When I commit transaction
    Then it success

  Scenario Outline: Lock not existed cloud
    When I execute query
    """
    SELECT *
      FROM code.lock_cloud(i_cloud_id => <cloud_id>)
    """
    Then it returns nothing

   Examples:
    | cloud_id |
    | NULL     |
    | -42      |

  Scenario: Cloud changes are logged in cloud revs
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => 'add-req-id'
    )
    """
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => 'up-usage-req-id'
    )
    """
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta       => code.make_quota(
            i_cpu       => -6.0,
            i_gpu       => -2,
            i_memory    => -512,
            i_ssd_space => -16000,
            i_hdd_space => -32000,
            i_clusters  => -1
        ),
        i_x_request_id => 'down-usage-req-id'
    )
    """
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_delta        => code.make_quota(
            i_cpu       => -6.0,
            i_gpu       => -2,
            i_memory    => -512,
            i_ssd_space => -16000,
            i_hdd_space => -32000,
            i_clusters  => -1
        ),
        i_x_request_id => 'down-quota-req-id'
    )
    """
    And successfully executed query
    """
    SELECT *
      FROM code.set_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_quota        => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => 'set-quota-req-id'
    )
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.clouds_revs
     WHERE cloud_id = (SELECT cloud_id FROM code.get_cloud(NULL, :cloud_ext_id))
    """
    Then it returns "5" rows matches
    """
    - cloud_rev: 1
      cpu_quota: 12.0
      gpu_quota: 4
      memory_quota: 1024
      ssd_space_quota: 32000
      hdd_space_quota: 64000
      clusters_quota: 2
      cpu_used: 0
      gpu_used: 0
      memory_used: 0
      ssd_space_used: 0
      hdd_space_used: 0
      clusters_used: 0
      x_request_id: add-req-id

    - cloud_rev: 2
      cpu_quota: 12.0
      gpu_quota: 4
      memory_quota: 1024
      ssd_space_quota: 32000
      hdd_space_quota: 64000
      clusters_quota: 2
      cpu_used: 12.0
      gpu_used: 4
      memory_used: 1024
      ssd_space_used: 32000
      hdd_space_used: 64000
      clusters_used: 2
      x_request_id: up-usage-req-id

    - cloud_rev: 3
      cpu_quota: 12.0
      gpu_quota: 4
      memory_quota: 1024
      ssd_space_quota: 32000
      hdd_space_quota: 64000
      clusters_quota: 2
      cpu_used: 6.0
      gpu_used: 2
      memory_used: 512
      ssd_space_used: 16000
      hdd_space_used: 32000
      clusters_used: 1
      x_request_id: down-usage-req-id

    - cloud_rev: 4
      cpu_quota: 6.0
      gpu_quota: 2
      memory_quota: 512
      ssd_space_quota: 16000
      hdd_space_quota: 32000
      clusters_quota: 1
      cpu_used: 6.0
      gpu_used: 2
      memory_used: 512
      ssd_space_used: 16000
      hdd_space_used: 32000
      clusters_used: 1
      x_request_id: down-quota-req-id

    - cloud_rev: 5
      cpu_quota: 12.0
      gpu_quota: 4
      memory_quota: 1024
      ssd_space_quota: 32000
      hdd_space_quota: 64000
      clusters_quota: 2
      cpu_used: 6.0
      gpu_used: 2
      memory_used: 512
      ssd_space_used: 16000
      hdd_space_used: 32000
      clusters_used: 1
      x_request_id: set-quota-req-id
    """

  Scenario: Set cloud quota
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.set_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_quota        => code.make_quota(
            i_cpu       => 24.0,
            i_gpu       => 8,
            i_memory    => 2048,
            i_ssd_space => 64000,
            i_hdd_space => 128000,
            i_clusters  => NULL
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cloud_ext_id: {{ cloud_ext_id }}
    cpu_quota: 24.0
    gpu_quota: 8
    memory_quota: 2048
    ssd_space_quota: 64000
    hdd_space_quota: 128000
    clusters_quota: 2
    cpu_used: 0.0
    gpu_used: 0
    memory_used: 0
    ssd_space_used: 0
    hdd_space_used: 0
    clusters_used: 0
    """

  Scenario: Set cloud quota beyond usage
    Given successfully executed query
    """
    SELECT *
      FROM code.add_cloud(
        i_cloud_ext_id  => :cloud_ext_id,
        i_quota         => code.make_quota(
            i_cpu       => 12.0,
            i_gpu       => 4,
            i_memory    => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters  => 2
        ),
        i_x_request_id => ''
    )
    """
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => (
            SELECT cloud_id
              FROM code.get_cloud(
                i_cloud_ext_id => :cloud_ext_id,
                i_cloud_id     => NULL)),
        i_delta       => code.make_quota(
            i_cpu => 12.0,
            i_gpu => 4,
            i_memory => 1024,
            i_ssd_space => 32000,
            i_hdd_space => 64000,
            i_clusters => 2
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.set_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_quota        => code.make_quota(
            i_cpu       => 10,
            i_gpu       => NULL,
            i_memory    => NULL,
            i_ssd_space => NULL,
            i_hdd_space => NULL,
            i_clusters  => NULL
        ),
        i_x_request_id => ''
    )
    """
    Then it fail with error matches "new row for relation .clouds. violates check constraint .check_quota."

  @fix
  Scenario Outline: Fix cloud usage
    . In some reasons cloud usage can be broken.
    . We should have tools that detect and fix it
    Given cloud with quota
    And folder
    And cluster with
    """
    type: clickhouse_cluster
    """
    And "clickhouse_cluster" subcluster
    And "<flavor>" flavor
    And "3" new hosts
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => :cloud_id,
        i_delta       => code.make_quota(
            i_cpu       => 42.0,
            i_memory    => 42,
            i_ssd_space => 42,
            i_hdd_space => 42,
            i_clusters  => 1
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.get_cloud_real_usage(:cloud_ext_id)
    """
    Then it returns one row matches
    """
    cpu: <cpu>
    memory: <memory>
    ssd_space: 126
    hdd_space: 0
    clusters: 1
    """
    When I execute query
    """
    SELECT *
      FROM code.fix_cloud_usage(:cloud_ext_id)
    """
    Then it success
    When I execute query
    """
    SELECT cpu_used,
           memory_used,
           ssd_space_used,
           hdd_space_used,
           clusters_used
      FROM code.get_cloud(
        i_cloud_ext_id => :cloud_ext_id,
        i_cloud_id     => NULL)
    """
    Then it returns one row matches
    """
    cpu_used: <cpu>
    memory_used: <memory>
    ssd_space_used: 126
    hdd_space_used: 0
    clusters_used: 1
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.clouds
     WHERE cloud_ext_id = :cloud_ext_id
       AND code.get_usage_from_cloud(clouds) != code.get_cloud_real_usage(cloud_ext_id)
    """
    Then it returns nothing

  Examples: Guarantee == Limit flavor
    | flavor    | cpu | memory |
    | flavor2   | 6   | 6      |

  Examples: Burstable flavor
    | flavor    | cpu | memory |
    | flavor2.5 | 6   | 6      |

  Examples: Burstable flavor with fractional cpu_used
    | flavor    | cpu | memory |
    | flavor0.5 | 1.5 | 3      |

  @fix @hadoop
  Scenario: Hadoop cluster don't increment usage
    . We should not count hadoop in any used field
    Given cloud with quota
    And "flavor1" flavor
    And folder
    And cluster with
    """
    type: clickhouse_cluster
    """
    And "clickhouse_cluster" subcluster
    And "3" new hosts
    And cluster with
    """
    type: hadoop_cluster
    """
    And "hadoop_cluster.masternode" subcluster
    And "1" new hosts
    And "hadoop_cluster.computenode" subcluster
    And "5" new hosts
    And "hadoop_cluster.datanode" subcluster
    And "7" new hosts
    When I execute query
    """
    SELECT *
      FROM code.get_cloud_real_usage(:cloud_ext_id)
    """
    Then it returns one row matches
    """
    cpu: 3.0
    memory: 3
    ssd_space: 126
    hdd_space: 0
    clusters: 1
    """


  @cpu_quota
  Scenario: CPU quota round
    . We use real for cpu_quota and cpu_used.
    . And we can get rounding error.
    . Like:
    . > SELECT cpu_used, cloud_id FROM dbaas.clouds WHERE cloud_id = 4;
    .   cpu_used
    . -------------
    .   2.23517e-08
    . (1 row)
    Given cloud with default quota
    And successfully executed query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id      => :cloud_id,
        i_delta         => code.make_quota(
            i_cpu       => 1.1,
            i_memory    => 0,
            i_ssd_space => 0,
            i_hdd_space => 0,
            i_clusters  => 1
        ),
        i_x_request_id => ''
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id      => :cloud_id,
        i_delta         => code.make_quota(
            i_cpu       => -1.0,
            i_memory    => 0,
            i_ssd_space => 0,
            i_hdd_space => 0,
            i_clusters  => 0
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cpu_used: 0.1
    """
    When I execute query
    """
    SELECT *
      FROM code.update_cloud_usage(
        i_cloud_id    => :cloud_id,
        i_delta       => code.make_quota(
          i_cpu       => -0.1,
          i_memory    => 0,
          i_ssd_space => 0,
          i_hdd_space => 0,
          i_clusters  => 0
        ),
        i_x_request_id => ''
    )
    """
    Then it returns one row matches
    """
    cpu_used: 0.0
    """
    When I execute query
    """
    SELECT cpu_used FROM code.get_cloud(
        i_cloud_id     => NULL,
        i_cloud_ext_id => :cloud_ext_id
    )
    """
    Then it returns one row matches
    """
    cpu_used: 0.0
    """
