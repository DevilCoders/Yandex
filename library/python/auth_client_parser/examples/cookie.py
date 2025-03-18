import auth_client_parser


def parse_cookie_zero_allocation():
    cookie = "3:1624446318.1.0.1234567890:ASDFGH:7f.100|111111.1.202.1:1234|m:YA_RU:23453.0M_i.abQ_s-kSWb0T646ff45"

    full_cookie = auth_client_parser.Cookie(cookie)
    assert full_cookie.status == auth_client_parser.ParseStatus.RegularMayBeValid

    session_info = full_cookie.session_info
    print(session_info)

    for user in full_cookie.users():
        print(user)

    default_user = full_cookie.default_user()
    print(default_user)
