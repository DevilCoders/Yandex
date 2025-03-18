import inspect


def generate_arcanum_url(depth=1):
    frame = inspect.currentframe()
    while depth > 0 and frame is not None:
        frame = frame.f_back
        depth -= 1
    assert frame is not None
    return 'https://arcanum.yandex-team.ru/arc_vcs/{0}#L{1}'.format(frame.f_code.co_filename, frame.f_lineno)
