cat > debian/changelog<<EOH
yamail-monrun-errata ($(git rev-list HEAD --count)-$(git rev-parse --short HEAD)) trusty; urgency=low

  * Yandex qutobuild

 -- ${USER} <${USER}@$(hostname)>  $(date +%a\,\ %d\ %b\ %Y\ %H:%M:%S\ %z)
EOH
