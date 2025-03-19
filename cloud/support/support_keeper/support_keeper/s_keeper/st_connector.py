from rest_framework import views
import requests, json
from rest_framework.response import Response
from rest_framework.permissions import AllowAny
import os 

class ExtAPI(views.APIView):
    """
    class to visit foreign apis
    """
    permission_classes = [AllowAny, ]
    def post(self, request):
        count_tickets_endpoint = 'https://st-api.yandex-team.ru/v2/issues/_count'
        st_token = os.environ.get('S_TOKEN', 'YouForgotYourToken') 
        headers = {
            'Authorization': f'OAuth {st_token}',
            'Content-Type': 'application/json'
        }

        req_type = request.data['req_type']
        response = {}
        # # dummy
        #
        # return Response({
        #     'tickets_open': 5,
        #     'tickets_in_progress': 10,
        #     'tickets_sla_failed': 2
        # })
        #
        # # end dummy

        if req_type == "query":
            ticket_types = {
                'tickets_open': 'Status: Open',
                'tickets_in_progress': 'Status: inProgress',
                'tickets_sla_failed': 'Status: Open, inProgress Filter: 426514'
            }
            for k, v in ticket_types.items():
                body = {
                    req_type: f"{request.data['tickets_open']} {v}"
                }
                response[k] = requests.post(count_tickets_endpoint,
                                            headers=headers,
                                            json=body)
        elif req_type == "filter":

            ticket_types = {
                'tickets_open': '"status": "Open"',
                'tickets_in_progress': '"status": "inProgress"',
                'tickets_sla_failed': '"status": "Open,inProgress"'
            }
            for k, v in ticket_types.items():
                body = {
                    req_type: json.loads(f"{request.data['tickets_open'][:-1]}, {v}}}")
                }

                response[k] = requests.post(count_tickets_endpoint,
                                            headers=headers,
                                            json=body)
        else:
            return Response(response)

        return Response(response)
