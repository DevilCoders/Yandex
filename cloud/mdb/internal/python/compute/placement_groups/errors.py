from cloud.mdb.internal.python.grpcutil import exceptions


class DuplicatePlacementGroupNameError(exceptions.AlreadyExistsError):
    """
    Name of a placement groups is unique.
    """

    compute_type = 'DuplicatePlacementGroupName'
