from cloud.mdb.internal.python.grpcutil import exceptions


class DuplicateDiskPlacementGroupNameError(exceptions.AlreadyExistsError):
    """
    Name of a disk placement groups is unique.
    """

    compute_type = 'DuplicateDiskPlacementGroupName'
