from redis_server_log_parser import ServerLogParser


def test_redis_parsing_valid_string():
    log_record = '3;3;3;2930:M 29 Dec 2018 16:11:02.861 * Running mode=standalone, port=6379.'
    parser = ServerLogParser()
    match_object = parser.parse_record(log_record)
    expected = {
        'key': '3;3;3;',
        'pid': '2930',
        'role': 'M',
        'datetime': '29 Dec 2018 16:11:02.861',
        'message': 'Running mode=standalone, port=6379.',
    }
    assert expected == match_object


def test_redis_parsing_invalid_string_fails():
    log_record = '3;3;3;2930:M 29 Dec 2018 16:11:02.861'
    parser = ServerLogParser()
    match_object = parser.parse_record(log_record)
    assert match_object is None
