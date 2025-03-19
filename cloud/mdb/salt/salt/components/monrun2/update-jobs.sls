monrun-jobs-update:
    cmd.wait:
        - name: 'chown -R monitor.monitor /etc/monrun && sudo -u monitor monrun --gen-jobs && service juggler-client restart || true'
