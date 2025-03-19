wu-client-req:
    test.nop

wu-client-ready:
    test.nop


{% set target_group = salt.pillar.get('yandex:environment') %}
{% set wsus = salt.pillar.get('data:wsus:address','')%}

wu-eleveate-nonadmins:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"
        - vname: "ElevateNonAdmins"
        - vdata: 1
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-target-group:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"
        - vname: "TargetGroup"
        - vdata: {{ target_group|yaml_encode }}
        - vtype: "REG_SZ"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-target-group-enabled:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"
        - vname: "TargetGroupEnabled"
        - vdata: 1
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-wserver:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"
        - vname: "WUServer"
        - vdata: {{ wsus|yaml_encode}}
        - vtype: "REG_SZ"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-status-wserver:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"
        - vname: "WUStatusServer"
        - vdata: {{ wsus|yaml_encode}}
        - vtype: "REG_SZ"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-auoptions:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "AUOptions"
{% if target_group == 'compute-prod' %}
        - vdata: 3 #download updates but not install
{% else %}
        - vdata: 4 #Automatic download and scheduled installation
{% endif %}
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

{% if target_group != 'compute-prod' %}
wu-reboot-with-users:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "NoAutoRebootWithLoggedOnUsers"
        - vdata: 0 #Automatic Updates notifies user that the computer will restart in 5 minutes.
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-scheduled-install-day:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "ScheduledInstallDay"
        - vdata: 0 #EveryDay
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-scheduled-install-ScheduledInstallTime:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "ScheduledInstallTime"
        - vdata: 12
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready
{% endif %}

wu-auto-install-minor-updates:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "AutoInstallMinorUpdates"
        - vdata: 1
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-detection-frequency:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "DetectionFrequency"
        - vdata: 24
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-detection-frequency-enabled:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "DetectionFrequencyEnabled"
        - vdata: 1
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-include-recommended-updates:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "IncludeRecommendedUpdates"
        - vdata: 0
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-no-autoupdate:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "NoAutoUpdate"
        - vdata: 0
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready

wu-use-wsus:
    reg.present:
        - name: "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"
        - vname: "UseWUServer"
        - vdata: 1
        - vtype: "REG_DWORD"
        - require:
            - test: wu-client-req
        - require_in:
            - test: wu-client-ready
