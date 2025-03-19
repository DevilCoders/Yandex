from startrek_client import Startrek


class Tracker(Startrek):
    def __init__(self, useragent, base_url, st_oauth, *args, **kwargs):
        super().__init__(useragent=useragent, base_url=base_url, token=st_oauth, *args, **kwargs)

    def get_ticket_by_issue_id(self, issue_id):
        query = f'Queue: "Поддержка Облака" and "Ticked ID": "{issue_id}"'
        result = [issue.key for issue in self.issues.find(query=query)]
        if not result:
            return None
        else:
            return result
