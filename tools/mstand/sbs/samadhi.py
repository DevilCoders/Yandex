SAMADHI_API_URL = "https://sbs.yandex-team.ru/api"

import yaqutils.requests_helpers as urequests


def get_sbs_workflow_id(sbs_ticket):
    """
    :type sbs_ticket: str
    :rtype: str
    """
    arr = sbs_ticket.split("-")
    assert len(arr) == 2 and arr[0] == "SIDEBYSIDE"
    sbs_ticket_number = int(arr[1])

    url = "{api}/experiment/{sbs_ticket_number}/workflow".format(api=SAMADHI_API_URL,
                                                                 sbs_ticket_number=sbs_ticket_number)
    try:
        raw_response = urequests.retry_request(method="GET", url=url, verify=False)
        json_result = raw_response.json()
        return json_result.get("id")
    except urequests.RequestPageNotFoundError:
        return None
