Overview
===
This library provides functionality for parsing 'Session_id' cookies and OAuth tokens

> &#10071; __NOTE: This library CANNOT be used for validation, it is solely meant for parsing and verifying the format__
> 
> Actual validation can only be provided through a request to blackbox.

* You can find some usage examples [here](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/auth_client_parser/examples)
* You can ask questions at:
  * [passport-dev@yandex-team.ru](mailto:passport-dev@yandex-team.ru)
  * [Telegram chat](https://t.me/joinchat/BfrKfUm4vsiHynwEIFszig)


OAuth Token
===
TOAuthToken
---
Extracts UID from OAuthToken in offline mode.

> &#10071; __NOTE: Cannot be used for validating OAuth tokens__

---
`TOAuthToken` allows:
1. `Parse` - to parse an oauth token
2. `Uid` - to get the uid from the token
---
> You should verify parsing success with the return of `Parse`

'Session_id' cookie parsers
===
Parse 'Session_id' cookies in offline mode

> &#10071; __NOTE: Parsing is NOT validation: actual validation can only be provided through a request to blackbox.__

> NOTE: Parse does NOT guarantee the same result in subsequent calls, as status will dependent on current time, cookie
> creation time and its ttl. If you wish to get consistent results, you can use `now` parameter to provide a reference

TFullCookie
---
Parses cookie and stores full cookie data in memory

---
`TFullCookie` allows:
1. `Parse` - to parse a cookie
2. `SessionInfo` - to get the session info
3. `DefaultUser` - to get the info of the default user
4. `Users` - to get the info of all the users in a cookie
---
You should verify cookie status with `Status`

> The details of EParseStatus are documented below

TZeroAllocationCookie
---
Extremely cheap cookie parsing, without storing full cookie data in memory

> NOTE: Shows only the default user from cookie: there may be several users.
>
> Should you need the information of more than one user, consider using TFullCookie.

---
1. `Parse` - to parse cookie
2. `SessionInfo` - to get session info
3. `User` - to get info of the default user
---
You should verify cookie status with `Status`

> The details of EParseStatus are documented below

Cookie data
===
EParseStatus
---
Cookie status

---
1. `Invalid` - cookie is invalid
2. `NoauthValid` - a valid noauth cookie
3. `RegularMayBeValid` - cookie has correct formatting but may or may NOT be valid (parser provides no such guarantee)
4. `RegularExpire` - cookie has correct format but has expired

TUserInfo
---
Per-user information structure

> `Lang_` - user's language
>
> User's language is a number from /usr/share/yandex/lang_detect_data.txt ( yandex-lang-detect-data.deb )

TSessionInfo
---
Per-session information structure

> `IsSafe` - obsolete, all cookies can be considered safe now
>
> `IsStress` - true if the cookie is used for stress-testing
