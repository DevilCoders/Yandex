from abc import ABC
from abc import abstractmethod


class MarketplaceMigrationBaseLogicalError(Exception, ABC):
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


class MigrationLockError(MarketplaceMigrationBaseLogicalError):
    message = "Lock error."

    def __init__(self, source, *args):
        super().__init__(*args)
        self.source = source


class MigrationPathNotFound(MarketplaceMigrationBaseLogicalError):
    message = "Can not build migration path."


class MigrationNoRollback(MarketplaceMigrationBaseLogicalError):
    message = "One of migrations in path has no rollback"


class MigrationPathNotCoincides(MarketplaceMigrationBaseLogicalError):
    message = "Migration path does not coincide."


class MigrationRegistryError(MarketplaceMigrationBaseLogicalError):
    message = "Runner in the registry."


class MigrationBadConfiguredError(MarketplaceMigrationBaseLogicalError):
    message = "Migration's files have bad configuration"
