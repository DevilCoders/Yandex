Feature: Check all our sql functions

    All our SQL STABLE or IMMUTABLE functions inlines theirs plans,
    cause it should be faster.

    If it fail on your function, check docs:
    https://wiki.postgresql.org/wiki/Inlining_of_SQL_functions

    Scenario: All our sql function inlines
        Given default database
        When I get all STABLE or IMMUTABLE SQL functions in schema "code"
        Then all their plans inlines
