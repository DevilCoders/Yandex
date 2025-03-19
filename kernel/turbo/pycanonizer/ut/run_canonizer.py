from turbo import CanonizeUrlForSaasDeprecatedLegacy
import unittest


class TestCanonizer(unittest.TestCase):
    def test_canonize(self):
        self.assertEqual(CanonizeUrlForSaasDeprecatedLegacy("http://www.a.com/test"), "a.com/test")
        self.assertEqual(CanonizeUrlForSaasDeprecatedLegacy("https://www.b.com"), "b.com")


if __name__ == '__main__':
    unittest.main()
