include:
  - nbs.client

# CLOUD-12821 this role is only used for fake compute-node which require nbs.service. Let us mock this service here
mocked-nbs-server:
  file.managed:
    - name: /etc/systemd/system/nbs.service
    - contents: |
       [Unit]
       Description = "Mock nbs server. Nothing to do"
       [Service]
       ExecStart=/bin/true
       RemainAfterExit=True

mock-nbs.service:
  service.running:
    - name: nbs
    - require:
      - file: mocked-nbs-server
