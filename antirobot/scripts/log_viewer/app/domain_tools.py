import urlparse

import __res
from devtools.fleur.util.path import GetSourcePath


class OwnerExtractor:
    def __init__(self, ld2_content, sep="\n"):
        self.ld2 = {line.rstrip() for line in ld2_content.split(sep)}

    def GetOwner(self, url):
        parts = urlparse.urlparse(url).netloc.split(".")
        if ".".join(parts[-2:]) not in self.ld2:
            return ".".join(parts[-2:])
        return ".".join(parts[-3:])


def GetYandexDomainFromHost(host):
    fields = host.split('.')

    posibleYandexPos = -3 if fields[-1].lower() == 'tr' else -2
    if fields[posibleYandexPos] == 'yandex':
        return '.'.join(fields[posibleYandexPos:])
    else:
        return None


def GetOwnerFromHost(host):
    if GetOwnerFromHost.OwnerExtractor is None:
        GetOwnerFromHost.OwnerExtractor = OwnerExtractor(__res.find("areas.lst"))
    return GetOwnerFromHost.OwnerExtractor.GetOwner("http://" + host) # GetOwner() required full url with scheme


GetOwnerFromHost.OwnerExtractor = None


def GetCookiesDomainFromHost(host):
    yandexDomain = GetYandexDomainFromHost(host)
    if yandexDomain is not None:
        return yandexDomain
    else:
        return GetOwnerFromHost(host)
