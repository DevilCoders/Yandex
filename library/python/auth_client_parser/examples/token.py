import auth_client_parser


def get_uid_from_token():
    token = "AQAAAADue-GoAAAI2wSUpddcm0BIk-r70vL1O_A"

    oauth_token = auth_client_parser.OAuthToken(token)
    assert oauth_token.ok
    return oauth_token.uid
