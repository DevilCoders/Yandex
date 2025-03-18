ARCADIA_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/trunk/"


def truncate_url(url):
    return url[len(ARCADIA_PREFIX):] if url.startswith(ARCADIA_PREFIX) else url


def generate_module_name(plugin_source):
    url = truncate_url(plugin_source.url)
    revision = str(plugin_source.revision)
    raw_name = url + "_" + revision
    return "".join([c if c.isalnum() else "_" for c in raw_name])
