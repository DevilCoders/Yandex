MAILTO=''
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
*/5 * * * * root (for container in `portoctl list -1`; do root=`portoctl get $container root`; if $(echo $root | grep -q /place/porto_volumes); then path="$root/etc/dom0hostname"; hostname -f >$path; fi; done) >/dev/null 2>&1
