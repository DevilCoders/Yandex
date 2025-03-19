Feature: Locks API

  Background:
    Given service is ready

  Scenario: Releasing non-existing lock fails
    When we release lock "non-existent"
    Then we fail with "Unable to find lock with id non-existent"

  Scenario: Creating lock without objects fails
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "bad lock",
        "objects": []
    }
    """
    Then we fail with "empty object list"

  Scenario: Creating lock with duplicate objects fails
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "bad lock",
        "objects": [
            "host1",
            "host1",
            "host2"
        ]
    }
    """
    Then we fail with "duplicate object host1"

  Scenario: Single lock is immediately acquired
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we get status of lock "test1"
    Then we get status
    """
    {
        "acquired": true,
        "conflicts": [],
        "holder": "func-test",
        "id": "test1",
        "objects": [
            "host1",
            "host2"
        ],
        "reason": "single lock"
    }
    """
    When we release lock "test1"
    Then we get no error

  Scenario: Retrying lock creation works
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    And we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we release lock "test1"
    Then we get no error

  Scenario: Retrying lock creation with non-matching objects order works
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    And we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host2",
            "host1"
        ]
    }
    """
    Then we get no error
    When we release lock "test1"
    Then we get no error

  Scenario: Creating duplicate lock with non-matching holder fails
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    And we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test2",
        "reason": "single lock",
        "objects": [
            "host2",
            "host1"
        ]
    }
    """
    Then we fail with "Found existing lock test1 with holder func-test"
    When we release lock "test1"
    Then we get no error

  Scenario: Creating duplicate lock with non-matching reason fails
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "first lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    And we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "second lock",
        "objects": [
            "host2",
            "host1"
        ]
    }
    """
    Then we fail with "Found existing lock test1 with reason first lock"
    When we release lock "test1"
    Then we get no error

  Scenario: Creating duplicate lock with non-matching objects fails
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    And we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "single lock",
        "objects": [
            "host3",
            "host2"
        ]
    }
    """
    Then we fail with "Found existing lock test1 with objects host1, host2"
    When we release lock "test1"
    Then we get no error

  Scenario: Lock with conflicts is acquired after conflicting lock is released
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "first lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we create lock with data
    """
    {
        "id": "test2",
        "holder": "func-test",
        "reason": "second lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we get status of lock "test2"
    Then we get status
    """
    {
        "acquired": false,
        "conflicts": [
            {
                "ids": [
                    "test1"
                ],
                "object": "host1"
            },
            {
                "ids": [
                    "test1"
                ],
                "object": "host2"
            }
        ],
        "holder": "func-test",
        "id": "test2",
        "objects": [
            "host1",
            "host2"
        ],
        "reason": "second lock"
    }
    """
    When we release lock "test1"
    Then we get no error
    When we get status of lock "test2"
    Then we get status
    """
    {
        "acquired": true,
        "conflicts": [],
        "holder": "func-test",
        "id": "test2",
        "objects": [
            "host1",
            "host2"
        ],
        "reason": "second lock"
    }
    """
    When we release lock "test2"
    Then we get no error

  Scenario: Lock listing works
    When we create lock with data
    """
    {
        "id": "test1",
        "holder": "func-test",
        "reason": "first lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we create lock with data
    """
    {
        "id": "test2",
        "holder": "func-test",
        "reason": "second lock",
        "objects": [
            "host1",
            "host2"
        ]
    }
    """
    Then we get no error
    When we list locks with data
    """
    {
        "holder": "func-test"
    }
    """
    Then we get locks list
    """
    {
        "locks": [
            {
                "id": "test1",
                "holder": "func-test",
                "reason": "first lock",
                "objects": [
                    "host1",
                    "host2"
                ]
            },
            {
                "id": "test2",
                "holder": "func-test",
                "reason": "second lock",
                "objects": [
                    "host1",
                    "host2"
                ]
            }
        ],
        "next_offset": "0"
    }
    """
    When we list locks with data
    """
    {
        "limit": 1
    }
    """
    Then we get locks list
    """
    {
        "locks": [
            {
                "id": "test1",
                "holder": "func-test",
                "reason": "first lock",
                "objects": [
                    "host1",
                    "host2"
                ]
            }
        ],
        "next_offset": "1"
    }
    """
    When we list locks with data
    """
    {
        "offset": 1
    }
    """
    Then we get locks list
    """
    {
        "locks": [
            {
                "id": "test2",
                "holder": "func-test",
                "reason": "second lock",
                "objects": [
                    "host1",
                    "host2"
                ]
            }
        ],
        "next_offset": "0"
    }
    """
    When we release lock "test2"
    Then we get no error
    When we release lock "test1"
    Then we get no error
