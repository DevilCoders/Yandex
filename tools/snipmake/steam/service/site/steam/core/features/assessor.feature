Feature: Assessor's action

     Scenario: Take available pack
        Given I am on the "usertasks/packs/" page
        When I click "Take pack" button
        Then I see a pack on "usertasks/current/" page

     Scenario: Complete available tasks
        Given I am on the "usertasks/current/" page
        When I complete available tasks
        Then I see a pack on "usertasks/finished/" page

     Scenario Outline: Check tabs
        Given I am on the "/" page
        When I click "<tab>" button
        Then I see "<subtab>" link
     Examples:
        | tab         | subtab |
        | Estimations | Tasks |

     Scenario Outline: Check permissions
        Given I am on the "<page>" page
        Then I see "<header>" header
     Examples:
        | page                                                                                    | header        |
        | /                                                                                       |               |
        | get_raw_file/0123456789012345678901234567890123456789012345678901234567890123/          | 403 Forbidden |
        | waiting_users/                                                                          | 403 Forbidden |
        | set_role/                                                                               | 403 Forbidden |
        | request_permission/                                                                     |               |
        | add_user/                                                                               |               |
        | querybins/                                                                              | 403 Forbidden |
        | querybins/add/0/                                                                        | 403 Forbidden |
        | querybins/delete/0/0/                                                                   | 403 Forbidden |
        | snippetpools/                                                                           | 403 Forbidden |
        | snippetpools/add/0/                                                                     | 403 Forbidden |
        | snippetpools/delete/0123456789012345678901234567890123456789012345678901234567890123/0/ | 403 Forbidden |
        | snippets/0123456789012345678901234567890123456789012345678901234567890123/              | 403 Forbidden |
        | taskpools/                                                                              | 403 Forbidden |
        | taskpools/add/                                                                          | 403 Forbidden |
        | taskpools/stop/0/0/                                                                     | 403 Forbidden |
        | taskpools/edit/0/                                                                       | 403 Forbidden |
        | taskpools/delete/0/0/                                                                   | 403 Forbidden |
        | taskpools/export/0/0/                                                                   | 403 Forbidden |
        | tasks/0/                                                                                | 403 Forbidden |
        | usertasks/current/                                                                      | User tasks    |
        | usertasks/questions/                                                                    | 403 Forbidden |
        | usertasks/packs/0/                                                                      | User tasks    |
        | usertasks/available/                                                                    | 403 Forbidden |
        | usertasks/aadmin_take/0/                                                                | 403 Forbidden |
        | usertasks/take_aadmin_task_batch/take/0/                                                | 403 Forbidden |
        | monitor/                                                                                | 403 Forbidden |
        | ajax/monitor/                                                                           | 403 Forbidden |
        | ajax/stop/01234567-0123-0123-0123-0123456789ab/                                         | 403 Forbidden |
        | ajax/fileupload/                                                                        | 403 Forbidden |
        | ajax/filedelete/                                                                        | 403 Forbidden |
#        | estimation/0/                                                                           | User tasks    |
        | estimation/check/0/                                                                     | 403 Forbidden |
        | estimation/process_errors/0/                                                            | 403 Forbidden |
#        | corrections/all/0/                                                                      | User tasks    |
        | send_email/0/                                                                           | 403 Forbidden |
        | statistics/0/                                                                           | 403 Forbidden |
