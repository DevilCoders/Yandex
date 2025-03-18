from .query_struct import SerpQueryInfo  # noqa
from .markup_struct import SerpMarkupInfo  # noqa
from .markup_struct import SerpUrlsInfo  # noqa


class ParsedSerp(object):
    def __init__(self, query_info, markup_info, urls_info):
        """
        :type query_info: SerpQueryInfo
        :type markup_info: SerpMarkupInfo
        :type urls_info: SerpUrlsInfo
        """
        self.query_info = query_info
        self.markup_info = markup_info
        self.urls_info = urls_info

    def is_failed(self):
        """
        :rtype: bool
        """
        return self.query_info.is_failed_serp

    def is_empty(self):
        """
        :rtype: bool
        """
        return self.query_info is None
