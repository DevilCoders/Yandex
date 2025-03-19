{# State which should be used for requisites #}

repositories-ready:
    cmd.run:
        - name: for i in $(seq 10); do apt-get clean; apt-get update && break; done
