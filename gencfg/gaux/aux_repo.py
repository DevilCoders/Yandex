"""
    Auxiliarily function to get specific information from repo.
"""

import re


def get_tags_by_tag_pattern(tag_pattern, repo):
    """
        Function filter all tags in <repo> to satisfy <tag_pattern> and return sorted list with last created tag as last list element.
        Variable <tag_pattern> return list of groups, which are used to sort tags in ascending order (e. g. for tags like stable-85-r25
        we will specify pattern ^stable-(\d+)/r(\d+)$ with two groups.

        :type tag_pattern: str
        :type repo: core.svnapi.SvnRepository

        :param tag_pattern: python regexp to filter suitable tags
        :param repo: svn repository

        :return (list of str): list of good tags sorted in ascending order
    """

    def order(tag):
        return map(lambda x: int(x), re.match(tag_pattern, tag).groups())

    filtered_tags = filter(lambda x: re.match(tag_pattern, x), repo.tags(timeout=3))
    filtered_tags.sort(cmp=lambda x, y: cmp(order(x), order(y)))

    return filtered_tags


def get_last_tag(tag_pattern, repo):
    """
        Return last tag that satisfy specified pattern
    """

    return get_tags_by_tag_pattern(tag_pattern, repo)[-1]
