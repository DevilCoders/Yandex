class ExtensionModifyHeadersException(RuntimeError):
    pass


class ExtensionModifyCookiesException(RuntimeError):
    pass


class ExtensionAdblockException(Exception):
    pass


class ExtensionInfoException(Exception):
    pass


class DownloadExtensionException(Exception):
    pass
