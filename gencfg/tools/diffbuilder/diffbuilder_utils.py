"""
    Aux utils for diffbuilder
"""

import os
import utils.standalone.get_gencfg_repo as get_gencfg_repo
from core.svnapi import SvnRepository


def construct_repo_from_name(tagname):
    """
        Checkout (if needed) repo and return core.svnapi.SvnRepository object
    """

    if tagname.startswith("tag@"):
        if os.path.exists(tagname):
            raise Exception("Found directory <%s>, started from prefix <tag@>: could not understand if we want to use this directory or checkout from remote repo")
        return SvnRepository(get_gencfg_repo.api_main(repo_type='full', tag=tagname.partition('@')[2]).path,
                             temporary=True)
    else:
        if not os.path.exists(tagname):
            raise Exception("Directory <%s> does not exists" % tagname)
        return SvnRepository(tagname)
