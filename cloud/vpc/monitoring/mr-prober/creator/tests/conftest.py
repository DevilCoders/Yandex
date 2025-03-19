import logging.config

import common.tests.conftest
import settings

test_database = common.tests.conftest.test_database
mocked_s3 = common.tests.conftest.mocked_s3
db = common.tests.conftest.db


# Setup logging for tests in the same way as for applications
logging.config.dictConfig(settings.LOGGING_CONFIG)
