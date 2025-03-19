cat > debian/changelog<<EOH
yamail-gpsync ($(git rev-list HEAD --count)-$(git rev-parse --short HEAD)) trusty; urgency=low

  * Yandex autobuild

 -- ${USER} <${USER}@$(hostname)>  $(date +%a\,\ %d\ %b\ %Y\ %H:%M:%S\ %z)
EOH
