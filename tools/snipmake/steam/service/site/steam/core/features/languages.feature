Feature: Switching languages

    Scenario: English to Russian
        Given My current language is "English":
        When I switch language
        Then I see Russian text

    Scenario: Russian to English
        Given My current language is "Русский":
        When I switch language
        Then I see English text
