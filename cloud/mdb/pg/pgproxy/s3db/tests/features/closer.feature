Feature: Testing s3_closer

  Scenario: Closing and opening replica
    When we run s3_closer on replica with command "close"
    Then one replica is closed
    Then s3_closer check output on replica contains:
    """
    1;Closed
    """
    When we run s3_closer on replica with command "open"
    Then all hosts are opened
    Then s3_closer check output on replica contains:
    """
    0;Host is open
    """

  Scenario: Not closing master, only with --force
    When we run s3_closer on master with command "close"
    Then all hosts are opened
    Then s3_closer check output on master contains:
    """
    0;Host is open
    """
    When we run s3_closer on master with command "close --force"
    Then s3_closer check output on master contains:
    """
    2;Closed
    """
    When we run s3_closer on master with command "open"
    Then all hosts are opened

  Scenario: Not closing --dry-run
    When we run s3_closer on replica with command "close --dry-run"
    Then s3_closer check output on replica contains:
    """
    0;Host is open
    """
