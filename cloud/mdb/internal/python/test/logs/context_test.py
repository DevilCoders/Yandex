import logging

from cloud.mdb.internal.python.logs import MdbLoggerAdapter


def test_default_extra_used(mocker):
    m_log = mocker.Mock()
    logger = MdbLoggerAdapter(m_log, dict(foo='bar'))
    logger.info('hello')
    m_log.log.assert_called_once_with(logging.INFO, 'hello', extra=dict(foo='bar'))


def test_override_context(mocker):
    m_log = mocker.Mock()
    logger = MdbLoggerAdapter(m_log, dict(foo='bar'))
    logger.copy_with_ctx(foo='baz', a=1).debug('from copy')
    m_log.log.assert_called_once_with(logging.DEBUG, 'from copy', extra=dict(foo='baz', a=1))

    logger.info('hello')
    m_log.log.assert_called_with(logging.INFO, 'hello', extra=dict(foo='bar'))


def test_as_context_manager(mocker):
    m_log = mocker.Mock()
    second_m_log = mocker.Mock()
    logger = MdbLoggerAdapter(m_log, dict(foo='bar'))
    second_logger = MdbLoggerAdapter(second_m_log, {})
    with logger.context(foo='baz', a=1):
        logger.debug('from ctx manager')
        m_log.log.assert_called_once_with(logging.DEBUG, 'from ctx manager', extra=dict(foo='baz', a=1))

        with logger.context(a=2):
            logger.debug('from second ctx manager')
            m_log.log.assert_called_with(logging.DEBUG, 'from second ctx manager', extra=dict(foo='baz', a=2))

        second_logger.debug('from ctx manager')
        second_m_log.log.assert_called_once_with(logging.DEBUG, 'from ctx manager', extra=dict(foo='baz', a=1))

    logger.info('hello')
    m_log.log.assert_called_with(logging.INFO, 'hello', extra=dict(foo='bar'))
    second_logger.info('hello')
    second_m_log.log.assert_called_with(logging.INFO, 'hello', extra=dict())
