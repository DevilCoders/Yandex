# yandex-media-mysqld-watchdog
watchdog for mysql restart and monitor
Can detect OOM or any other failed mysql; writes log, restart mysql
Usage:

# check for restarts
mysqld_watchdog.sh check
# fix mysql if it is down
mysqld_watchdog.sh fix

Repository: media-common
