def get_token():
    with open('.st_token', 'r') as f:
        token = f.read()
    return token.strip()

