Feature: Assessor administrator's action

     Scenario: Check inspection tasks count
        Given I am on the "taskpools/" page
        When I go to the first td page
        Then I see that assessor is underchecked less than 2 tasks

     Scenario: Check tasks
        Given I am on the "usertasks/available/" page
        And Taskpool is finished
        When I take and check available tasks
        Then I see "User tasks" header

     Scenario Outline: Check tabs
        Given I am on the "/" page
        When I click "<tab>" button
        Then I see "<subtab>" link
     Examples:
        | tab         | subtab      |
        | Estimations | Task pools  |
        | Estimations | Tasks       |
        | Estimations | Estimations |
        | Estimations | Statistics  |

     Scenario Outline: Check permissions
        Given I am on the "<page>" page
        Then I see "<header>" header
     Examples:
        | page                                                                                    | header               |
        | /                                                                                       |                      |
        | get_raw_file/0123456789012345678901234567890123456789012345678901234567890123/          | 403 Forbidden        |
        | waiting_users/                                                                          | 403 Forbidden        |
        | set_role/                                                                               | 403 Forbidden        |
        | request_permission/                                                                     |                      |
        | add_user/                                                                               |                      |
        | querybins/                                                                              | 403 Forbidden        |
        | querybins/add/0/                                                                        | 403 Forbidden        |
        | querybins/delete/0/0/                                                                   | 403 Forbidden        |
        | snippetpools/                                                                           | 403 Forbidden        |
        | snippetpools/add/0/                                                                     | 403 Forbidden        |
        | snippetpools/delete/0123456789012345678901234567890123456789012345678901234567890123/0/ | 403 Forbidden        |
        | snippets/0123456789012345678901234567890123456789012345678901234567890123/              | 403 Forbidden        |
        | taskpools/                                                                              | Task pools           |
#        | taskpools/add/                                                                          | 403 Forbidden        |
        | taskpools/stop/0/0/                                                                     | Task pools           |
        | taskpools/edit/0/                                                                       | Page not found (404) |  # Task pools |
        | taskpools/delete/0/0/                                                                   | 403 Forbidden        |
#        | taskpools/export/0/0/                                                                   | Task pools           |
        | tasks/0/                                                                                | Task pools           |
        | usertasks/current/                                                                      | User tasks           |
        | usertasks/questions/                                                                    | User tasks           |
        | usertasks/packs/                                                                        | 403 Forbidden        |
        | usertasks/available/                                                                    | User tasks           |
#        | usertasks/aadmin_take/0/                                                                | User tasks           |
        | usertasks/take_aadmin_task_batch/take/0/                                                | User tasks           |
        | monitor/                                                                                | 403 Forbidden        |
        | ajax/monitor/                                                                           | 403 Forbidden        |
        | ajax/stop/01234567-0123-0123-0123-0123456789ab/                                         | 403 Forbidden        |
#        | ajax/fileupload/                                                                        |                      |
#        | ajax/filedelete/                                                                        |                      |
#        | estimation/0/                                                                           | User tasks           |
#        | estimation/check/0/                                                                     | User tasks           |
#        | estimation/process_errors/0/                                                            | User tasks           |
#        | corrections/all/0/                                                                      | User tasks           |
#        | send_email/0/                                                                           | User tasks           |
#        | statistics/0/                                                                           | Task pools           |
