Feature: Testing s3.to_keyrange function

    Scenario: Chunk with ordinary borders
        When we check lower and upper border of chunk
        """
        start_key: test1
        end_key: test2
        """
        Then we get following result
        """
        lower: test1
        upper: test2
        """

    Scenario: Chunk with infinity lower border
        When we check lower and upper border of chunk
        """
        start_key:
        end_key: test2
        """
        Then we get following result
        """
        lower:
        upper: test2
        """

    Scenario: Chunk with infinity upper border
        When we check lower and upper border of chunk
        """
        start_key: test1
        end_key:
        """
        Then we get following result
        """
        lower: test1
        upper:
        """

    Scenario: Chunk with infinity borders
        When we check lower and upper border of chunk
        """
        start_key:
        end_key:
        """
        Then we get following result
        """
        lower:
        upper:
        """

    Scenario: Bad symbols in borders
        When we check lower and upper border of chunk
        """
        start_key: 0"][)("',
        end_key: 1"][)("',
        """
        Then we get following result
        """
        lower: 0"][)("',
        upper: 1"][)("',
        """
