UNION()

OWNER(g:antirobot)

BUNDLE(
    antirobot/cbb/cbb_django/project/wsgi NAME cbb
    antirobot/cbb/cbb_django/project/manage NAME cbb_manage
    antirobot/cbb/cbb_fast
)

NEED_CHECK()

END()
