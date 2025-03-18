# -*- coding: utf-8 -*-
from __future__ import unicode_literals


STAFF = {
    'development': {
        'protocol': 'https',
        'host': 'staff-api.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'staff-api.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'staff-api.yandex-team.ru',
    },
}

STARTREK_API = {
    'development': {
        'protocol': 'https',
        'host': {
            'intranet': 'st-api.test.yandex-team.ru',
            'other': 'tracker-api.test.qloud.yandex.ru',
        }
    },
    'testing': {
        'protocol': 'https',
        'host': {
            'intranet': 'st-api.test.yandex-team.ru',
            'other': 'tracker-api.test.qloud.yandex.ru',
        }
    },
    'production': {
        'protocol': 'https',
        'host': {
            'intranet': 'st-api.yandex-team.ru',
            'other': 'tracker-api.qloud.yandex.ru',
        }
    },
}

STARTREK = {
    'development': {
        'protocol': 'http',
        'host': {
            'intranet': 'st.test.yandex-team.ru',
            'other': 'tracker.test.yandex.ru',
        }
    },
    'testing': {
        'protocol': 'http',
        'host': {
            'intranet': 'st.test.yandex-team.ru',
            'other': 'tracker.test.yandex.ru',
        }
    },
    'production': {
        'protocol': 'http',
        'host': {
            'intranet': 'st.yandex-team.ru',
            'other': 'tracker.yandex.ru',
        }
    },
}

CALENDAR = {
    'development': {
        'protocol': 'http',
        'host': 'calendar.yandex.ru',
    },
    'testing': {
        'protocol': 'http',
        'host': 'calendar.yandex.ru',
    },
    'production': {
        'protocol': 'http',
        'host': 'calendar.yandex.ru',
    },
}

CALENDAR_INTERNAL = {
    'development': {
        'protocol': 'https',
        'host': 'calendar-api.testing.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'calendar-api.testing.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'calendar-api.tools.yandex.net',
    },
}

PLAN_API = {
    'development': {
        'protocol': 'http',
        'host': 'startrek-back01gt.yandex.net:8089',
    },
    'testing': {
        'protocol': 'http',
        'host': 'startrek-back01gt.yandex.net:8089',
    },
    'production': {
        'protocol': 'http',
        'host': 'plan-api.http.yandex.net',
    },
}

PLAN = {
    'development': {
        'protocol': 'https',
        'host': 'plan-api.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'plan-api.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'plan-api.yandex-team.ru',
    },
}

OAUTH = {
    'development': {
        'protocol': {'intranet': 'https',
                     'other': 'http',
                     },
        'host': {
            'intranet': 'oauth.yandex-team.ru',
            'other': 'oauth-internal.yandex.ru',
        },
    },
    'testing': {
        'protocol': 'https',
        'host': {
            'intranet': 'oauth.yandex-team.ru',
            'other': 'oauth-test.yandex.ru',
        },
    },
    'production': {
        'protocol': {'intranet': 'https',
                     'other': 'http',
                     },
        'host': {
            'intranet': 'oauth.yandex-team.ru',
            'other': 'oauth-internal.yandex.ru',
        },
    },
}

FORMATTER = {
    'development': {
        'protocol': 'https',
        'host': 'wf.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'wf.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'wf.yandex-team.ru',
    },
}

AT = {
    'development': {
        'protocol': 'https',
        'host': 'at-back-testing.tools.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'at-back-testing.tools.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'at-back-production.tools.yandex-team.ru',
    },
}

ORANGE = {
    'development': {
        'protocol': 'https',
        'host': 'orange-api.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'orange-api.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'orange-api.yandex-team.ru',
    },
}

GAP = {
    'development': {
        'protocol': 'http',
        'host': 'gap.test.tools.yandex-team.ru',
    },
    'testing': {
        'protocol': 'http',
        'host': 'gap.test.tools.yandex-team.ru',
    },
    'production': {
        'protocol': 'http',
        'host': 'gap.yandex-team.ru',
    },
}

GAP2 = {
    'development': {
        'protocol': 'https',
        'host': 'staff.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'staff.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'staff.yandex-team.ru',
    },
}

JIRA = {
    'development': {
        'protocol': 'https',
        'host': 'jira.test.tools.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'jira.test.tools.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'jira.yandex-team.ru',
    }
}

INFLECTOR = {
    'development': {
        'protocol': 'http',
        'host': 'hamzard.yandex.net:8891'
    },
    'testing': {
        'protocol': 'http',
        'host': 'reqwizard.yandex.net:8891'
    },
    'production': {
        'protocol': 'http',
        'host': 'reqwizard.yandex.net:8891'
    }
}

UATRAITS = {
    'development': {
        'protocol': 'http',
        'host': 'uatraits.qloud.yandex.ru',
    },
    'testing': {
        'protocol': 'http',
        'host': 'uatraits.qloud.yandex.ru',
    },
    'production': {
        'protocol': 'http',
        'host': 'uatraits.qloud.yandex.ru',
    },
}

GEOBASE = {
    'development': {
        'protocol': 'http',
        'host': 'geobase.qloud.yandex.ru',
    },
    'testing': {
        'protocol': 'http',
        'host': 'geobase.qloud.yandex.ru',
    },
    'production': {
        'protocol': 'http',
        'host': 'geobase.qloud.yandex.ru',
    },
}

GOALS = {
    'development': {
        'protocol': 'https',
        'host': 'goals.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'goals.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'goals.yandex-team.ru',
    },
}

INTRASEARCH = {
    'development': {
        'protocol': 'https',
        'host': {
            'intranet': 'search-back.test.yandex-team.ru',
            'other': 'connect-test.ws.yandex.ru',
        }
    },
    'testing': {
        'protocol': 'https',
        'host': {
            'intranet': 'search-back.test.yandex-team.ru',
            'other': 'connect-test.ws.yandex.ru',
        }
    },
    'preprod': {
        'protocol': 'https',
        'host': {
            'intranet': 'search-back.yandex-team.ru',
            'other': 'connect-integration-qa.ws.yandex.ru',
        }
    },
    'production': {
        'protocol': 'https',
        'host': {
            'intranet': 'search-back.yandex-team.ru',
            'other': 'connect.yandex.ru',
        }
    },
}

DIRECTORY = {
    'development': {
        'protocol': 'https',
        'host': 'api-internal-test.directory.ws.yandex.net',
    },
    'testing': {
        'protocol': 'https',
        'host': 'api-internal-test.directory.ws.yandex.net',
    },
    'preprod': {
        'protocol': 'https',
        'host': 'api-integration-qa.directory.ws.yandex.net',
    },
    'production': {
        'protocol': 'https',
        'host': 'api-internal.directory.ws.yandex.net',
    },
}

TVM = {
    'development': {
        'protocol': 'https',
        'host': 'tvm-api.yandex.net',
    },
    'testing': {
        'protocol': 'https',
        'host': 'tvm-api.yandex.net',
    },
    'production': {
        'protocol': 'https',
        'host': 'tvm-api.yandex.net',
    },
}

ABC = {
    'development': {
        'protocol': 'https',
        'host': 'abc-back.test.yandex-team.ru',
    },
    'testing': {
        'protocol': 'https',
        'host': 'abc-back.test.yandex-team.ru',
    },
    'production': {
        'protocol': 'https',
        'host': 'abc-back.yandex-team.ru',
    },
}

CHECKFORM = {
    'development': {
        'protocol': 'http',
        'host': 'checkform2-test.n.yandex-team.ru',
    },
    'testing': {
        'protocol': 'http',
        'host': 'checkform2-test.n.yandex-team.ru',
    },
    'preprod': {
        'protocol': 'http',
        'host': 'checkform2-test.n.yandex-team.ru',
    },
    'production': {
        'protocol': 'http',
        'host': 'checkform.so.yandex-team.ru',
    },
}
