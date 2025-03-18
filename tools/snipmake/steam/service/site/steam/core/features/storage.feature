Feature: Store/get file in storage

    Scenario: Simple string
        Given I have a string "Test string"
        When I store it
        Then I get back the same content
