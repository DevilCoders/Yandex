from abc import ABC

from cloud.marketplace.common.yc_marketplace_common.utils.errors import MarketplaceBaseLogicalError


class SendMailBaseError(MarketplaceBaseLogicalError, ABC):
    pass


class SendMailAuthenticationError(SendMailBaseError):
    message = "Authentication failed"


class SendMailSendError(SendMailBaseError):
    message = "Send failed"
