Feature: Assessor administrator's parallel action

     Scenario: Check inspection tasks count
        Given base aadmin is "robot-steam-aa"
        And additional aadmins are:
          | login    |
          | robot-steam-dv |
        When all aadmins have same tasks with estimation value {"informativity": "1", "content_richness": "0", "readability": "-1"}
        And all aadmins recheck (shuffle digits on 1) first 5 tasks parallel for 60 times
        Then all aadmins estimation with value {"informativity": "1", "content_richness": "0", "readability": "-1"}
