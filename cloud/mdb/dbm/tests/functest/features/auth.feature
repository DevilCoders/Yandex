Feature: Authorization works correctly

    Scenario Outline: Auth with token
        Given a deployed DBM
         When we issue "GET /containers" with:
            """
            timeout: 2
            headers:
                Authorization: OAuth <token>
            """
          Then we get response with code <code>

        Examples: Request with <status> token for user <user> returns <code>
          | user  | status   | token                                | code |
          | alice | valid    | 11111111-1111-1111-1111-111111111111 |  200 |
          | alice | disabled | 11111111-1111-1111-1111-000000000000 |  403 |
          | alice | invalid  | 11111111-0000-0000-0000-111111111111 |  403 |
          | bob   | valid    | 22222222-2222-2222-2222-222222222222 |  200 |
          | eva   | valid    | 33333333-3333-3333-3333-333333333333 |  403 |


    Scenario Outline: Auth with cookie
        Given a deployed DBM
         When we issue "GET /volumes" with:
            """
            headers:
                Cookie: Session_id=<sessionid>
            """
          Then we get response with code <code>

        Examples: Request with <status> Session_id for user <user> returns <code>
          | user  | status   | sessionid                            | code |
          | alice | valid    | aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa |  200 |
          | alice | invalid  | aaaaaaaa-aaaa-aaaa-aaaa-000000000000 |  307 |
          | bob   | valid    | bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb |  200 |
          | eva   | valid    | cccccccc-cccc-cccc-cccc-cccccccccccc |  403 |
