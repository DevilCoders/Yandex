# coding: utf-8

from tools_structured_logs.logic.log_records.vendors.base_vendor import IVendorInstrumenter
from ..info_provider import RequestsLibDomainProvider


def make_response_hook(domain_provider):
    """
    @type domain_provider: RequestsLibDomainProvider
    """

    def response_hook(response, *args, **kwargs):
        domain_provider.set_context(request=response.request, response=response).make_records()

    return response_hook


class SessionObjectInstrumenter(IVendorInstrumenter):
    sessions = None

    def __init__(self, *sessions):
        self.sessions = sessions
        super(SessionObjectInstrumenter, self).__init__()

    def instrument(self):
        provider = RequestsLibDomainProvider()
        for session in self.sessions:
            session.hooks['response'].append(make_response_hook(provider))
