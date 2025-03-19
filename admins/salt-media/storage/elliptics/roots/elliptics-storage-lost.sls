/usr/bin/make_fio_job.sh:
  file.managed:
    - source: salt://files/elliptics-storage-lost/usr/bin/make_fio_job.sh
    - mode: 755
