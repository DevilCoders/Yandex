# -*- coding: utf-8 -*-

from collections import namedtuple


Pages = namedtuple('Pages', ['first', 'prev', 'pages_range', 'next',
                             'last', 'active_page'])


def get_pages(cur_page, pages_count, pages_in_window=5):
    left_page = cur_page
    right_page = cur_page + 1
    is_changed = True
    while right_page - left_page < pages_in_window and is_changed:
        is_changed = False
        if left_page > 0:
            left_page -= 1
            is_changed = True
        if right_page < pages_count:
            right_page += 1
            is_changed = True
    p_first = 0
    if cur_page == p_first:
        p_first = None
    p_last = pages_count - 1
    if cur_page == p_last:
        p_last = None
    p_prev = cur_page - pages_in_window
    if p_prev < 0:
        p_prev = None
    p_next = cur_page + pages_in_window
    if p_next >= pages_count:
        p_next = None
    return Pages(p_first, p_prev, range(left_page, right_page), p_next,
                 p_last, cur_page)
