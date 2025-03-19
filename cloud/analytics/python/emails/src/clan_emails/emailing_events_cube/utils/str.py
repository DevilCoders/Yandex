def b2s(bytes_str):
    res = ''
    try:
        res = bytes_str.decode('utf-8')
    except Exception:
        pass
    return res