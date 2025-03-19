import random
import string
import hashlib

try:
    import six
except ImportError:
    from salt.ext import six


def sha2_crypt(password, rounds=5000, salt=None, salt_len=20):
    # https://akkadia.org/drepper/SHA-crypt.txt
    if not salt:
        salt = get_random_alphanumeric_string(salt_len)

    if len(salt) > salt_len:
        salt = salt[:salt_len]

    password = ensure_binary(password)
    salt = ensure_binary(salt)

    a = hashlib.sha256()
    a.update(password)
    a.update(salt)

    b = hashlib.sha256()
    b.update(password)
    b.update(salt)
    b.update(password)

    _div, _mod = divmod(len(password), 32)
    for _ in range(_div):
        a.update(b.digest())

    a.update(b.digest()[:_mod])

    len_pass = len(password)
    while len_pass > 0:
        if len_pass % 2 == 1:
            a.update(b.digest())
        else:
            a.update(password)
        len_pass = len_pass >> 1

    dp = hashlib.sha256()
    for _ in range(len(password)):
        dp.update(password)

    p = b''
    for i in range(_div):
        p += dp.digest()
    p += dp.digest()[:_mod]

    ds = hashlib.sha256()
    for _ in range(16 + ensure_ord(a.digest()[0])):
        ds.update(salt)

    s = b''
    _div, _mod = divmod(len(salt), 32)
    for _ in range(_div):
        s += ds.digest()
    s += ds.digest()[:_mod]

    c = a
    for i in range(rounds):
        tmp_c = hashlib.sha256()
        if i % 2 == 1:
            tmp_c.update(p)
        else:
            tmp_c.update(c.digest())

        if i % 3 != 0:
            tmp_c.update(s)

        if i % 7 != 0:
            tmp_c.update(p)

        if i % 2 == 0:
            tmp_c.update(p)
        else:
            tmp_c.update(c.digest())
        c = tmp_c
    result = '$5$'
    # ___
    if rounds != 5000:
        result += 'rounds={0}$'.format(rounds)
    result += ensure_str(salt) + '$'
    encoding_map = (
        (0, 10, 20),
        (21, 1, 11),
        (12, 22, 2),
        (3, 13, 23),
        (24, 4, 14),
        (15, 25, 5),
        (6, 16, 26),
        (27, 7, 17),
        (18, 28, 8),
        (9, 19, 29),
        (-1, 31, 30),
    )

    for line in encoding_map:
        if line[0] == -1:
            result += bytes_2_base64(0, c.digest()[line[1]], c.digest()[line[2]], 3)
        else:
            result += bytes_2_base64(c.digest()[line[0]], c.digest()[line[1]], c.digest()[line[2]], 4)

    return result


def bytes_2_base64(a, b, c, n):
    alphabet = './0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
    result = ''

    w = ensure_ord(a) << 16 | ensure_ord(b) << 8 | ensure_ord(c)
    for i in range(n):
        result += alphabet[w & 0x3F]
        w >>= 6
    return result


def get_random_alphanumeric_string(length):
    letters_and_digits = string.ascii_letters + string.digits
    return ''.join((random.choice(letters_and_digits) for _ in range(length)))


def get_auth_string(password, auth_plugin, salt=None):
    if auth_plugin == 'mysql_native_password':
        # auth string = '*{sha1(sha1(password))}'
        return '*' + hashlib.sha1(hashlib.sha1(ensure_binary(password)).digest()).hexdigest().upper()
    elif auth_plugin == 'caching_sha2_password':
        # auth string = '$A$005${salt}{hash}
        # 'A' means sha256 algorithm
        # '005' means 5000 rounds in sha crypt
        sha256_auth_string = get_auth_string(password, 'sha256_password', salt=salt)
        return '$A$005$' + sha256_auth_string[3:].replace('$', '')
    elif auth_plugin == 'sha256_password':
        # hashing algorithm http://www.akkadia.org/drepper/SHA-crypt.txt
        # auth string = '$5${salt}${hash}'
        # 5 means 5000 rounds in sha crypt
        return sha2_crypt(password, salt=salt)
    return None


def ensure_ord(byte_or_int):
    if six.PY2:
        return ord(bytes(byte_or_int))

    return byte_or_int


def ensure_binary(s, encoding='utf-8', errors='strict'):
    """Coerce **s** to six.binary_type.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> encoded to `bytes`
      - `bytes` -> `bytes`
    NOTE: The function equals to six.ensure_binary that is not present in the salt version of six module.
    """
    if isinstance(s, six.binary_type):
        return s
    if isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    raise TypeError("not expecting type '%s'" % type(s))


def ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s
