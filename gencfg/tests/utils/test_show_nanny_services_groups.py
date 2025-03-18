import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.settings import SETTINGS
from utils.common import show_nanny_services_groups

# def test_dev_nanny(curdb):
#    result = show_nanny_services_groups.jsmain({
#        'oauth_key' : SETTINGS.services.nanny.rest.devoauth,
#        'nanny_url' : SETTINGS.services.nanny.rest.devurl,
#    })

#    print result

#    filtered_result = filter(lambda x: x.name == 'kimkim_test', result)

#    assert len(filtered_result) == 1
#    assert filtered_result[0].ticket_queue == 'KIMKIM_TEST'
#    assert filtered_result[0].gencfg_groups == ['MSK_RESERVED']
