import base64

from Crypto.Cipher import AES


def hash_string(data):
    return str(sum(ord(ch) * (i + 1) for i, ch in enumerate(data)))


def decrypt(data, crypt_key):
    cipher = AES.new(crypt_key[:16], AES.MODE_ECB)
    decoded = base64.b64decode(data)
    return cipher.decrypt(decoded).strip()


def encrypt(data, crypt_key):
    cipher = AES.new(crypt_key[:16], AES.MODE_ECB)
    data = data.ljust((len(data) // 16 + 1) * 16, " ")
    encrypted = cipher.encrypt(data)
    return base64.b64encode(encrypted)


def get_crypted_cookie_value(crypt_key, ip, user_agent, accept_language, generate_time):
    data = "{}\t{}\t{}\t{}".format(generate_time, hash_string(ip), hash_string(user_agent), hash_string(accept_language))
    return encrypt(data, crypt_key)
