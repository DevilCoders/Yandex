from django.http import JsonResponse
from django.conf import settings
import os
import re
import yt.wrapper as yt

from antirobot.cbb.cbb_django.cbb.library import antiddos


GROUP_CHECK_RE = re.compile(" *(doc|cgi|ident_type|arrival_time|service_type|country_id) *= *#[0-9]+ *")


def check_ip_from(rule):
    try:
        _, flag_id = rule.split("=")
        flag_id = int(flag_id.strip("/"))
    except:
        return False

    return True


def ajax_antiddos_check(request):
    if not request.method == "POST":
        return JsonResponse({"status": "POST only"})

    rules = request.POST["rules"]
    ip_from_rule = None
    req_text = request.POST["request"]

    cleaned_rules = []
    ignored_checks = []

    for rule in rules.split(";"):
        if rule.startswith("ip_from="):
            ip_from_rule = rule
            ignored_checks.append("ip_from")
        else:
            group_check = GROUP_CHECK_RE.fullmatch(rule)

            if group_check is None:
                cleaned_rules.append(rule)
            else:
                ignored_checks.append(group_check.group(1))

    rules = ";".join(cleaned_rules)

    correct_syntax = False
    correct_length = False
    request_valid = False
    request_matches = False

    if len(rules) <= 1024:
        correct_length = True

    if rules and antiddos.check_rules(rules):
        if ip_from_rule is None or check_ip_from(ip_from_rule):
            correct_syntax = True

        if correct_syntax and req_text and antiddos.validate_request(req_text):
            request_valid = True

            if request_valid and antiddos.check_request(rules, req_text):
                request_matches = True

    return JsonResponse({
        "correct_syntax": correct_syntax,
        "correct_length": correct_length,
        "request_valid": request_valid,
        "request_matches": request_matches,
        "has_ip_from": bool(ip_from_rule),
        "ignored_checks": ignored_checks,
    })


def ajax_get_random_requests(request):
    client = yt.YtClient(proxy="hahn", token=os.getenv("YT_TOKEN") or yt.http_helpers.get_token())

    result = {}
    for row in client.read_table(settings.RANDOM_REQUESTS_TABLE):
        result[row["service"]] = row["requests"]

    return JsonResponse(result)
