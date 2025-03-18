Feature: Developer Actions

    Scenario: Upload QueryBin
       Given I am on the "querybins" page
       When I click "Add new" button
       When I specify the title "test"
       When I upload file "querybin.txt"
       When I submit the form
       Then I see QueryBin "test"

    Scenario: Check QueryBin
       Given I am on the "raw_files" page
       When I download first file
       Then downloaded file is equal to "querybin.txt"

    Scenario: Upload SnippetPool
       Given I am on the "snippetpools" page
       When I click "Add new" button
       When I specify the title "uploaded"
       When I upload SnippetPool "snippetpool.xml"
       When I submit the form
       Then I see SnippetPool "uploaded"
       And snippet count is equal to "snippetpool.xml"

    Scenario: Delete SnippetPool
       Given I am on the "snippetpools" page
       When I delete first element
       Then I see "No snippet pools are available." paragraph

    Scenario Outline: Download SERP
       Given I am on the "snippetpools" page
       When I click "Add new" button
       When I specify the title "<title>"
       When I specify the QueryBin "test, Russia"
       When I specify the host "yandex.ru"
       When I specify CGI-params "<params>"
       When I submit the form
       Then I see SnippetPool "<title>"
    Examples:
       | title        | params                    |
       | serp_with_al | snip=exps=active_learning |
       | serp         |                           |

    Scenario: Delete QueryBin
       Given I am on the "querybins" page
       When I delete first element
       Then I see "No query bins are available." paragraph

    Scenario: Create TaskPool for deletion
       Given I am on the "taskpools" page
       When I create TaskPool "test_delete"
       Then I see TaskPool "test_delete"

    Scenario: TaskPool deletion
       Given I am on the "taskpools" page
       When I delete first element
       Then I see "No taskpools are available." paragraph

    Scenario: Create TaskPool for estimations
       Given I am on the "taskpools" page
       When I create TaskPool "test"
       Then I see TaskPool "test"

    Scenario: TaskPool starting
       Given I am on the "taskpools" page
       When I click play button
       Then I see stop button

    Scenario Outline: Check tabs
       Given I am on the "/" page
       When I click "<tab>" button
       Then I see "<subtab>" link
    Examples:
       | tab           | subtab                  |
       | Requests      | Query bins              |
       | Snippet pools | Snippet pools           |
       | Snippet pools | Background task monitor |
       | Estimations   | Task pools              |
       | Estimations   | Tasks                   |
       | Estimations   | Estimations             |
       | Estimations   | Statistics              |
       | Management    | Waiting users           |
       | Management    | Roles management        |
       | Management    | Files in storage        |

    Scenario Outline: Check permissions
       Given I am on the "<page>" page
       Then I see "<header>" header
    Examples:
       | page                                                                                    | header                  |
       | /                                                                                       |                         |
       | get_raw_file/0123456789012345678901234567890123456789012345678901234567890123/          |                         |
       | waiting_users/                                                                          | Waiting users           |
       | set_role/                                                                               | Roles                   |
       | request_permission/                                                                     |                         |
       | add_user/                                                                               |                         |
       | querybins/                                                                              | Query bins              |
       | querybins/add/0/                                                                        | Query bins              |
       | querybins/delete/0/0/                                                                   | Page not found (404)    |  # Query bins |
       | snippetpools/                                                                           | Snippet pools           |
       | snippetpools/add/0/                                                                     | Snippet pools           |
       | snippetpools/delete/0123456789012345678901234567890123456789012345678901234567890123/0/ | Snippet pools           |
       | snippets/0123456789012345678901234567890123456789012345678901234567890123/              | Snippet pools           |
       | taskpools/                                                                              | Task pools              |
#       | taskpools/add/                                                                          | 403 Forbidden           |
       | taskpools/stop/0/0/                                                                     | Task pools              |
       | taskpools/edit/0/                                                                       | Page not found (404)    |  # Task pools |
       | taskpools/delete/0/0/                                                                   | Task pools              |
#       | taskpools/export/0/0/                                                                   | Task pools              |
       | tasks/0/                                                                                | Task pools              |
       | usertasks/current/                                                                      | User tasks              |
       | usertasks/questions/                                                                    | User tasks              |
       | usertasks/packs/                                                                        | 403 Forbidden           |
       | usertasks/available/                                                                    | User tasks              |
#       | usertasks/aadmin_take/0/                                                                | User tasks              |
       | usertasks/take_aadmin_task_batch/take/0/                                                | User tasks              |
       | monitor/                                                                                | Background task monitor |
#       | ajax/monitor/                                                                           | Background task monitor |
       | ajax/stop/01234567-0123-0123-0123-0123456789ab/                                         | 403 Forbidden           |
#       | ajax/fileupload/                                                                        |                         |
#       | ajax/filedelete/                                                                        |                         |
#       | estimation/0/                                                                           | User tasks              |
#       | estimation/check/0/                                                                     | User tasks              |
#       | estimation/process_errors/0/                                                            | User tasks              |
#       | corrections/all/0/                                                                      | User tasks              |
#       | send_email/0/                                                                           | User tasks              |
#       | statistics/0/                                                                           | Task pools              |
