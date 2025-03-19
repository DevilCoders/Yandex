#!/bin/bash

dpkg -l | awk '{print $2}' > /tmp/packages_local
curl -s http://c.yandex-team.ru/api-cached/packages_on_host/$(hostname -f) | awk '{print $1}' > /tmp/packages_deploy

check_package_on_conductor () {
    package=$1
    curl -s http://c.yandex-team.ru/api/generator/mmcleric.package_info?package=${package} > /tmp/packages_current
    package_version_testing=$(cat /tmp/packages_current | grep ^testing | awk '{print $2}')
    package_version_prestable=$(cat /tmp/packages_current | grep ^prestable | awk '{print $2}')
    package_version_stable=$(cat /tmp/packages_current | grep ^stable | awk '{print $2}')
    if [[ ${package_version_testing} != "" ]] || [[ ${package_version_prestable} != "" ]] || [[ ${package_version_stable} != "" ]];
	then echo "${package} is on conductor (but not at deploy): s: ${package_version_stable}; p: ${package_version_prestable}; t: ${package_version_testing}"; 
    fi
}

check_package_on_deploy () {
    package=$1
    cat /tmp/packages_deploy | grep 1>/dev/null ^${package}$ || return 1
}


for i in `cat /tmp/packages_local`; do 
    check_package_on_deploy $i || check_package_on_conductor $i; 
done


