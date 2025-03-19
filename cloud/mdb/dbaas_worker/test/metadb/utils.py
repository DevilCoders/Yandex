import os


def get_recipe_dsn_from_env(dbname: str) -> dict[str, str]:
    """
    Get metadb env vars
    """
    dbname = dbname.upper()
    dsn = {}
    for env_var, dsn_var in {
        f'{dbname}_POSTGRESQL_RECIPE_HOST': 'host',
        f'{dbname}_POSTGRESQL_RECIPE_PORT': 'port',
    }.items():
        try:
            dsn[dsn_var] = os.environ[env_var]
        except KeyError:
            raise RuntimeError(f"Variable '{env_var}' not found")
    return dsn
