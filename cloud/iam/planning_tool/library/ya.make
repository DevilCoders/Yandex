OWNER(g:cloud-iam)

PY3_LIBRARY()

PEERDIR(
    contrib/python/boto3
    contrib/python/colorama
    contrib/python/isodate
    contrib/python/pytz
    contrib/python/requests
    contrib/python/PyYAML
    contrib/python/Jinja2
    contrib/python/beautifulsoup4
    contrib/python/httmock
)

PY_SRCS(
    __init__.py
    absences.py
    clients.py
    config.py
    daily.py
    holidays.py
    monthly.py
    tasks.py
    tax.py
    timesheet.py
    tools.py
    worklog.py
    report.py
    utils.py
)

RESOURCE_FILES(
    PREFIX cloud/iam/planning_tool/library/
    templates/base.html
    templates/daily.html
    templates/monthly.html
    templates/timesheet.html
    templates/tasks.html
    templates/tax.wiki
    templates/common_macros.html
    templates/monthly_macros.html
    templates/daily_macros.html
)

RESOURCE(
    cloud/iam/planning_tool/library/common_config.yaml common_config
)

END()
