#!/usr/bin/env python3
from yc_monitoring import JugglerPassiveCheck, SystemdService, JugglerPassiveCheckException

VALID_UPTIME = 10
KIKIMR_SERVICE_NAME = "kikimr.service"


def check_is_kikimr_running(check: JugglerPassiveCheck):
    srv_props = SystemdService.parameters(KIKIMR_SERVICE_NAME)

    # service not found on host
    if srv_props.get("LoadState") in ("not-found", "masked"):
        check.ok("Node without kikimr")
        return

    # service in wrong state
    state = srv_props.get("ActiveState")
    sub_state = srv_props.get("SubState")
    if not (state == "active" and sub_state == "running"):
        check.crit("Kikimr service in wrong state: {}({})".format(state, sub_state))
        return

    uptime_service = SystemdService.unit_uptime_sec(KIKIMR_SERVICE_NAME, srv_props)
    if uptime_service < VALID_UPTIME:
        check.warn('Uptime kikimr is {} seconds'.format(uptime_service))
        return


def main():
    check = JugglerPassiveCheck("kikimr")
    try:
        check_is_kikimr_running(check)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
