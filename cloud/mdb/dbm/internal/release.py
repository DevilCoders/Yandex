from library.python import svn_version


def get_release() -> str:
    """
    Get application version
    """
    return f'1.{svn_version.svn_revision()}'
