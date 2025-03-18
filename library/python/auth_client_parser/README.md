Overview
===

> This library is a binding for C++ library with Cython.

This library provides functionality for parsing 'Session_id' cookies and OAuth tokens

> &#10071; __NOTE: This library CANNOT be used for validation, it is solely meant for parsing and verifying the format__
>
> Actual validation can only be provided through a request to blackbox.

* You can view the original library [here](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/auth_client_parser)
* You can find some usage examples [here](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/auth_client_parser/examples)
* You can ask questions at:
  * [passport-dev@yandex-team.ru](mailto:passport-dev@yandex-team.ru)
  * [Telegram chat](https://t.me/joinchat/BfrKfUm4vsiHynwEIFszig)


OAuth Token
===
OAuthToken
---
Extracts UID from OAuthToken in offline mode.

> &#10071; __NOTE: Cannot be used for validating OAuth tokens__

---
`OAuthToken` allows:
1. `__init__` - to parse an oauth token
2. `uid` - to get the uid from the token
---
> You should verify parsing success with `ok`

'Session_id' cookie parsers
===
Parse 'Session_id' cookies in offline mode

> &#10071; __NOTE: Parsing is NOT validation: actual validation can only be provided through a request to blackbox.__

> NOTE: Parse does NOT guarantee the same result in subsequent calls, as status will dependent on current time, cookie
> creation time and its ttl. If you wish to get consistent results, you can use `now` parameter to provide a reference
Cookie
---
Parses cookie and stores full cookie data in memory

---
`Cookie` allows:
1. `__init__` - to parse a cookie
2. `session_info` - to get the session info
3. `default_user` - to get the info of the default user
4. `users` - to get the info of all the users in a cookie
---
You should verify cookie status with `status`

> The details of ParseStatus are documented below

Cookie data
===
ParseStatus
---
Cookie status

---
1. `Invalid` - cookie is invalid
2. `NoauthValid` - a valid noauth cookie
3. `RegularMayBeValid` - cookie has correct formatting but may or may NOT be valid (parser provides no such guarantee)
4. `RegularExpire` - cookie has correct format but has expired

UserInfo
---
Per-user information structure

> `lang` - user's language
>
> User's language is a number from /usr/share/yandex/lang_detect_data.txt ( yandex-lang-detect-data.deb )

SessionInfo
---
Per-session information structure

> `safe` - obsolete, all cookies can be considered safe now
>
> `stress` - true if the cookie is used for stress-testing
