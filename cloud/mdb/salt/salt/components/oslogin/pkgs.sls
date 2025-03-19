oslogin-packages:
  pkg.installed:
    - pkgs:
      - google-compute-engine-oslogin: "20210429.00-0ubuntu1~18.04.0"
    - require:
        - test: oslogin-pkgs-req
    - require_in:
        - test: oslogin-pkgs-done

disable_google-oslogin-cache.timer:
  service.disabled:
    - name: google-oslogin-cache.timer
    - require:
      - pkg: oslogin-packages
    - require_in:
        - test: oslogin-pkgs-done

stop_google-oslogin-cache.timer:
  service.dead:
    - name: google-oslogin-cache.timer
    - require:
      - pkg: oslogin-packages
    - require_in:
        - test: oslogin-pkgs-done

oslogin-pkgs-req:
  test.nop

oslogin-pkgs-done:
  test.nop
