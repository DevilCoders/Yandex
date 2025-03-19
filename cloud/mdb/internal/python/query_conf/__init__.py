import pkg_resources
import os


def load_queries(package: str, queries_dir: str) -> dict[str, str]:
    """
    Load queries from resources
    """
    ret = {}
    for query_file in pkg_resources.resource_listdir(package, queries_dir):
        query_name = os.path.splitext(query_file)[0]
        ret[query_name] = str(pkg_resources.resource_string(package, queries_dir + '/' + query_file), encoding='ascii')
    return ret
