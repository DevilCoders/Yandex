import unittest
from datetime import datetime, timedelta
from time import mktime
from uuid import uuid4

from yc_auth_token import AuthType, SignedToken, Token, Principal, TokenVersion, is_expired


class TokenTest(unittest.TestCase):

    def test_create_token(self):
        self.assert_(self._create_test_token())

    def test_parse_token(self):
        token = self._create_test_token()
        signed_token = SignedToken()
        signed_token.token = token.SerializeToString()
        serialized_token = signed_token.SerializeToString()

        parsed_signed_token = SignedToken()
        parsed_signed_token.ParseFromString(serialized_token)
        parsed_token = Token()
        parsed_token.ParseFromString(parsed_signed_token.token)
        self.assertEqual(parsed_token, token)

    def test_is_expired(self):
        token = self._create_test_token()
        signed_token = SignedToken()
        signed_token.token = token.SerializeToString()
        serialized_token = signed_token.SerializeToString()
        self.assertEqual(is_expired(serialized_token), False)

        token = self._create_test_token()
        token.expires_at = token.issued_at
        signed_token = SignedToken()
        signed_token.token = token.SerializeToString()
        serialized_token = signed_token.SerializeToString()
        self.assertEqual(is_expired(serialized_token), True)

    @staticmethod
    def _create_test_token():
        token = Token()
        token.version = TokenVersion.Value("v1")
        token.id = str(uuid4())
        issue_time = datetime.now()
        expire_time = issue_time + timedelta(hours=12)
        token.issued_at = int(mktime(issue_time.timetuple()))
        token.expires_at = int(mktime(expire_time.timetuple()))
        token.auth_type = AuthType.Value("TOKEN")

        principal = token.principal
        principal.id = "yp:123123"
        principal.name = "test-user"
        principal.type = Principal.Type.Value("USER")

        return token


if __name__ == '__main__':
    unittest.main()
