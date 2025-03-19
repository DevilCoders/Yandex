cleanup docker env:
  cron.present:
    - name: docker system prune --volumes --force --all
    - user: root
    - minute: 01
    - hour: 00
    - daymonth: '*'
    - dayweek: 7 # Sunday
    - month: '*'
    - comment: 'Weekly Docker Environment cleanup - CLOUD-24680'
