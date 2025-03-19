"""API error exceptions."""

from abc import ABC
from abc import abstractmethod

from yc_common.exceptions import ApiError
from yc_common.exceptions import BadRequestError
from yc_common.exceptions import InternalServerError
from yc_common.exceptions import LogicalError
from yc_common.exceptions import RequestConflictError
from yc_common.exceptions import ResourceNotFoundError


class MarketplaceBaseError(ApiError, ABC):
    @property
    @abstractmethod
    def message(self):
        pass

    def __init__(self):
        if issubclass(self.__class__, InternalServerError):
            super().__init__()
        else:
            super().__init__(code=self.__class__.__name__, message=self.message)


class MarketplaceBaseLogicalError(LogicalError, ABC):
    @property
    @abstractmethod
    def message(self):
        pass

    def __init__(self, *args):
        if args:
            message, args = args[0], args[1:]

            if args:
                message = message % args
        else:
            message = self.message

        super().__init__(message)


class MarketplaceResourceNotFoundError(MarketplaceBaseError, ResourceNotFoundError):
    @property
    @abstractmethod
    def message(self):
        pass


class MarketplaceRequestConflictError(MarketplaceBaseError, RequestConflictError):
    @property
    @abstractmethod
    def message(self):
        pass


class MarketplaceBadRequestError(MarketplaceBaseError, BadRequestError):
    @property
    @abstractmethod
    def message(self):
        pass


class MarketplaceInternalServerError(MarketplaceBaseError, InternalServerError):
    @property
    @abstractmethod
    def message(self):
        pass


class ReadOnlyError(MarketplaceRequestConflictError):
    message = "Service is read only now"


class InvalidPartnerRequestTypeError(MarketplaceBadRequestError):
    message = "Invalid partner request type."


class InvalidPartnerRequestIdError(MarketplaceResourceNotFoundError):
    message = "Invalid partner request ID."


class InvalidPublisherIdError(MarketplaceResourceNotFoundError):
    message = "Invalid publisher ID."


class PublisherConflictError(MarketplaceRequestConflictError):
    message = "Publisher attributes conflict."


class InvalidIsvIdError(MarketplaceResourceNotFoundError):
    message = "Invalid isv ID."


class IsvConflictError(MarketplaceRequestConflictError):
    message = "Isv attributes conflict."


class InvalidVarIdError(MarketplaceResourceNotFoundError):
    message = "Invalid var ID."


class VarConflictError(MarketplaceRequestConflictError):
    message = "Var attributes conflict."


class InvalidTaskIdError(MarketplaceResourceNotFoundError):
    message = "Invalid task id."


class SaasProductIdError(MarketplaceResourceNotFoundError):
    message = "Invalid SaaS product ID."


class SimpleProductIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace product ID."


class OsProductIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace product ID."


class AvatarIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace image ID."


class EulaIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace EULA ID."


class OsProductFamilyIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace product family ID."


class OsProductDuplicateError(MarketplaceRequestConflictError):
    message = "There is marketplace product with same ID."


class InvalidProductType(MarketplaceBadRequestError):
    message = "Invalid marketplace product type."


class InvalidComputeImageStatus(MarketplaceBadRequestError):
    message = "Compute Image must be in READY status."


class InvalidStatus(MarketplaceBadRequestError):
    message = "Invalid status."


class OsProductFamilyBrokenError(MarketplaceInternalServerError):
    message = "Family is broken."


class OsProductFamilyVersionIdError(MarketplaceResourceNotFoundError):
    message = "Invalid marketplace product family version ID."


class OsProductFamilyVersionDuplicateError(MarketplaceRequestConflictError):
    message = "There is marketplace product version with same ID."


class OsProductFamilyVersionStatusUpdateError(MarketplaceInternalServerError):
    message = "Status can not be changed."


class OsProductFamilyVersionUpdateError(MarketplaceInternalServerError):
    message = "Version can not be changed."


class InvalidProductFamilyDeprecationError(MarketplaceBadRequestError):
    message = "Invalid deprecation status."


class InvalidRelatedProductError(MarketplaceBadRequestError):
    message = "Invalid related product billing account."


class InvalidCategoryIdError(MarketplaceResourceNotFoundError):
    message = "Invalid category ID."


class FailedToUploadImage(MarketplaceBadRequestError):
    message = "Failed to upload image to MDS."


class ImageFieldRequired(MarketplaceBadRequestError):
    message = "Must provide image."


class FileFieldRequired(MarketplaceBadRequestError):
    message = "Must provide file."


class MalformedFilename(MarketplaceBadRequestError):
    message = "Filename should contain file extension."


class S3UploadCredentialsNotSet(MarketplaceInternalServerError):
    message = "S3 upload credentials are missing."


class ImageUnknownSizePrefix(MarketplaceBadRequestError):
    message = "Unknown image size prefix."


class ImageUploadError(MarketplaceBadRequestError):
    message = "Failed to upload image to storage."


class ImageInvalidMimeType(MarketplaceBadRequestError):
    message = "Unsupported image mimetype."


class InvalidFileMimeType(MarketplaceBadRequestError):
    message = "Unsupported file mimetype."


class MigrationPathNotFound(MarketplaceBaseLogicalError):
    message = "Can not build migration path."


class MigrationNoRollback(MarketplaceBaseLogicalError):
    message = "One of migrations in path has no rollback"


class MigrationPathNotCoincides(MarketplaceBaseLogicalError):
    message = "Migration path does not coincide."


class MigrationRegistryError(MarketplaceBaseLogicalError):
    message = "Runner in the registry."


class MigrationBadConfiguredError(MarketplaceBaseLogicalError):
    message = "Migration's files have bad configuration"


class FormNotFoundError(MarketplaceResourceNotFoundError):
    message = "Form not found."


class SkuDraftNotFoundError(MarketplaceResourceNotFoundError):
    message = "SKU Draft not found."


class SkuDraftNameConflictError(MarketplaceRequestConflictError):
    message = "There is SKU Draft with same name."


class MetricsFolderIdError(MarketplaceBadRequestError):
    message = "Bad folder ids in metrics."


class BlueprintNotFoundError(MarketplaceResourceNotFoundError):
    message = "Blueprint id not found."


class BlueprintInvalidStatus(MarketplaceBadRequestError):
    message = "Blueprint is not in Active status."


class BlueprintBuildLinksNotFound(MarketplaceBadRequestError):
    message = "Blueprint has no build links."


class BlueprintTestLinksNotFound(MarketplaceBadRequestError):
    message = "Blueprint has no test suites links."


class BuildNotFoundError(MarketplaceResourceNotFoundError):
    message = "Build id not found."
