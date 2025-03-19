Feature: Get info about dataproc resource presets

  @cli @resource-presets
  Scenario: List resource presets
    When we run YC CLI
    """
    yc dataproc resource-presets list
    """
    Then YC CLI exit code is 0

    When save YC CLI response as resource_presets var
    And filter resource_presets list var with
    """
    id: s2.medium
    """
    Then var resource_presets equals
    """
    - id: s2.medium
      zone_ids:
      - ru-central1-a
      - ru-central1-b
      - ru-central1-c
      cores: "8"
      memory: "34359738368"
    """


  @cli @resource-presets
  Scenario: Get resource preset info
    When we run YC CLI
    """
    yc dataproc resource-presets get s2.medium
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    id: s2.medium
    zone_ids:
    - ru-central1-a
    - ru-central1-b
    - ru-central1-c
    cores: "8"
    memory: "34359738368"
    """
