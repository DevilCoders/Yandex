"""Generic constants."""


class ServiceNames:
    IDENTITY = "identity"
    SAMPLE = "gauthling_test"
    COMPUTE = "compute"
    COMPUTE_INTERNAL = "compute-internal"
    MARKETPLACE = "marketplace"
    MARKETPLACE_INTERNAL = "marketplace_internal"


SECOND_MILLISECONDS = 1000
"""Number of milliseconds in second."""


MINUTE_SECONDS = 60
"""Number of seconds in minute."""

HOUR_SECONDS = 60 * MINUTE_SECONDS
"""Number of seconds in hour."""

DAY_SECONDS = 24 * HOUR_SECONDS
"""Number of seconds in day."""

WEEK_SECONDS = 7 * DAY_SECONDS
"""Number of seconds in week."""


KILOBYTE = 1024
"""Number of bytes in kilobyte."""

MEGABYTE = 1024 * KILOBYTE
"""Number of bytes in megabyte."""

GIGABYTE = 1024 * MEGABYTE
"""Number of bytes in gigabyte."""

TERABYTE = 1024 * GIGABYTE
"""Number of bytes in terabyte."""

PETABYTE = 1024 * TERABYTE
"""Number of bytes in petabyte."""


IDEMPOTENCE_HEADER = "Idempotency-Key"


DATABASE_REQUEST_TIMEOUT = 40
"""Hope that such a big timeout is temporary - until we solve constant DEADLINE_EXCEEDED errors in KiKiMR."""

SERVICE_REQUEST_TIMEOUT = DATABASE_REQUEST_TIMEOUT + 5
"""Base timeout for services that use KiKiMR."""
