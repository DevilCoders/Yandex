#!/usr/bin/env bash
set -e
#set -x

usage()
{
cat << EOF
usage: $0 options

OPTIONS:
   -h          Show this message
   -f          URL to deb package
   -r          Repository on dist.yandex.ru
   -p          Version postfix
   -a          "0" disable autorestart service. Default "1"
   -s          Sign key uid
   -e | -E     Script to download and run after package extraction
   -k          GPG passphrase
   -R          Use following symbol as name record separator
EOF
}

#url=''
repository=''
version_postfix='yandex0'
autorestart='1'
package_name_rs='_'

while getopts 'f:r:p:a:e:s:E:k:R:' OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         f)
             file=$OPTARG
             ;;
         r)
             repository=$OPTARG
             ;;
         p)
             version_postfix=$OPTARG
             ;;
         a)
             autorestart=$OPTARG
             ;;
         e)
             postextract_hook=$OPTARG
             ;;
         s)
             sign_uid=$OPTARG
             ;;
         E)
             postextract_hook_file=$OPTARG
             ;;
         k)
             key=$OPTARG
             ;;
         R)
             package_name_rs=$OPTARG
             ;;
         ?)
             usage
             exit
             ;;
     esac
done

if [[ -z $file ]]; then
     usage
     exit 1
fi

if [[ -z $repository ]]; then
     usage
     exit 1
fi

wget $file
if [[ ! -z $file ]]; then
     spackage=$(ls *.deb)
fi

# Package may contain RS in its name, so count indices backwards and cut off known parts: architecture and version. Rest is the name.
package=$(echo $spackage | awk -F "${package_name_rs}" '{print substr($0, 1, index($0,$(NF-1))-2)}')
# Version is always second from last
version=$(echo $spackage | awk -F "${package_name_rs}" '{print $(NF-1)}')
# Arch (and whatever else, file extension) is the last
arch=$(echo $spackage | awk -F "${package_name_rs}" '{print $(NF)}' | sed 's/\.deb//')

output_package_name="${package}_${version}+${version_postfix}_${arch}"

echo "Extract package content"
dpkg-deb -x $spackage ${package}/

echo "Extract package control"
dpkg-deb -e $spackage ${package}/DEBIAN

if [[ ! -z $postextract_hook ]]; then
    echo "Run postextract hook"
    wget -O "postextract_hook.sh" "$postextract_hook"
    bash -x postextract_hook.sh
fi

if [[ ! -z $postextract_hook_file ]]; then
    echo "Run postextract hook"
    bash -x "$postextract_hook_file"
fi

echo "Extract package changelog"
changelog="${package}/usr/share/doc/${package}/changelog.Debian.gz"
if [[ ! -f $changelog ]]; then
   changelog=$(find ${package}/usr/share/doc -name 'changelog*.gz' 2>/dev/null| head -n 1)
else
    echo "changelog not found"
fi
if [[ -z $changelog ]] || [[ ! -f $changelog ]]; then
    rm -f $changelog
    dch --create --force-distribution --distribution "unstable" --package "$package" --newversion "${version}+${version_postfix}" -c changelog 'Rebuild package'
    gzip -c changelog > changelog.gz
    changelog="${package}/usr/share/doc/${package}/changelog.gz"
    if [ -e $changelog ] ; then
        mkdir -p $(dirname ${changelog}) || true
        mv changelog.gz $changelog
    fi
else
    zcat $changelog > changelog
    echo "Update changelog"
    dch -b --package "$package" --newversion "${version}+${version_postfix}" -c changelog 'Rebuild package' --distribution "unstable"
    gzip -c changelog > $changelog
fi

# replace md5sum for changelog in md5sums file
md5file="${package}/DEBIAN/md5sums"
if [[ -f $md5file ]] && [[ -f $changelog ]]; then
    changelog_base=$(basename $changelog)
    if grep -q $changelog_base $md5file 2>/dev/null; then
    md5_new=$(md5sum $changelog | cut -f1 -d' ' )
        md5_old=$(grep $changelog_base $md5file  | cut -f1 -d' ')
        sed -i $md5file -e "s/$md5_old/$md5_new/g"
    else
    md5sum $changelog >> $md5file
    sed -i $md5file -e "s#${package}/##g"
    fi
else
    echo "md5file and changelog not found"
fi

if [ $autorestart -eq 0 ]; then
    echo "Disable restart"
    for f in $(ls ${package}/DEBIAN/{pre,post}*); do
        sed -i 's/invoke-rc.d/: #/' $f
    done
fi

# fix control
section=$(awk '/Section:/ {print $NF}' ${package}/DEBIAN/control 2>/dev/null)
[[ -z "$section" ]] && section='misc'
priority=$(awk '/Priority:/ {print $NF}' ${package}/DEBIAN/control 2>/dev/null)
[[ -z "$priority" ]] && priority='extra'
echo "${output_package_name}.deb $section $priority" > files
echo "Source: ${package}" > control
grep -iv "Source:" ${package}/DEBIAN/control >> control
cp control ${package}/DEBIAN/control

echo "Build package"
sed -i "s/$version/$version+$version_postfix/g" ${package}/DEBIAN/control

fakeroot dpkg-deb --root-owner-group -D -b ${package}/ ./

echo "Generate changes"
dpkg-genchanges -b -u'.' -f'files' -c'control' -l'changelog' > ${output_package_name}.changes

echo "Sign package"
gpg --list-keys
debsign -k${sign_uid} *.changes

echo "Dupload package"
#dupload --to $repository -f --nomail *.changes
#
scp $(ls *$version_postfix*) ${repository}.dupload.dist.yandex.ru:/repo/${repository}/mini-dinstall/incoming/

# We need a pause between upload and dmove
sleep 60

for i in *$version_postfix*.deb
do
    pkg_name=$(echo "$i" | rev | cut -d_ -f3- | rev)
    pkg_version=$(echo "$i" | rev | cut -d_ -f2 | rev)
    ssh robot-pgaas-ci@duploader.yandex.ru sudo dmove "$repository" stable "$pkg_name" "$pkg_version" unstable || exit "$?"
done

rm -rf ${package}/
