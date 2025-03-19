"""
Exception classes
"""


class MdbMongoToolsException(Exception):
    """
    Main exception
    """


class LocalLockNotAcquired(MdbMongoToolsException):
    """
    Could not acquire local lock
    """


class ReplSetInfoGatheringError(MdbMongoToolsException):
    """
    Replica set information gathering error
    """


class ReplSetInfoPrimaryNotFound(ReplSetInfoGatheringError):
    """
    Replica set information: primary was not found
    """


class ReplSetInfoMultiplePrimariesFound(ReplSetInfoGatheringError):
    """
    Replica set information: multiple primaries were found
    """


class ResetupException(MdbMongoToolsException):
    """
    Main resetup exception
    """


class PredictorOperationNotAllowed(ResetupException):
    """
    Required operation is not allowed
    """


class QuorumLoss(PredictorOperationNotAllowed):
    """
    Required operation leads to quorum loss
    """


class EtaTooLong(PredictorOperationNotAllowed):
    """
    Required operation eta is long
    """


class FreshSecondariesNotEnough(PredictorOperationNotAllowed):
    """
    Count of caught up secondaries is not enough
    """


class ResetupTimeoutExceeded(ResetupException):
    """
    Resetup timeout exceeded
    """


class ResetupUnexpectedError(ResetupException):
    """
    Resetup unexpected error
    """


class StepdownException(MdbMongoToolsException):
    """
    Main stepdown exception
    """


class StepdownPrimaryWaitTimeoutExceeded(StepdownException):
    """
    Stepdown timeout exceeded
    """


class StepdownTimeoutExceeded(StepdownException):
    """
    Stepdown timeout exceeded
    """


class StepdownUnexpectedError(StepdownException):
    """
    Stepdown unexpected error
    """


class StepdownSamePrimaryElected(StepdownException):
    """
    Same primary elected after stepdown
    """


class GetterException(MdbMongoToolsException):
    """
    Main getter exception
    """


class GetterFailure(GetterException):
    """
    Getter failure exception
    """


class GetterUnexpectedError(GetterException):
    """
    Getter unexpected error
    """
