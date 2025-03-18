#!/usr/local/bin/bash

TEMPROOT="/hol/antirobot/temp"
TEMPDIR="/hol/antirobot/temp/data"
DATA="/hol/antirobot/data"
PACKNAME="antirobot_data.tar.gz"

FILELIST=(IPREG.detailed L-cookie-keys.txt geodata3.bin whitelist privileged_ips)

element_count=${#FILELIST[@]}

getmd5 ()
{
    local OS=`uname -s`
    local checksum
    local _file
    _file="$1"

    if [[ $OS == 'Linux'  ]]; then
        checksum=`/usr/bin/md5sum $_file | /usr/bin/awk '{ print $1 }' `
    else
        checksum=`md5 -q $_file`
    fi


    echo $checksum
}

equiv_check ()
{
	local _file
	local _tfilesumm
	local _pfilesumm
	_file="$1"


        if [ -s $DATA/${_file} ]; then
		 #_tfilesumm=`md5 -q $TEMPDIR/${_file}`
         _tfilesumm=$(getmd5 $TEMPDIR/${_file})
		 #_pfilesumm=`md5 -q $DATA/${_file}`
         _pfilesumm=$(getmd5 $DATA/${_file})
			if [ "X$_tfilesumm" = "X$_pfilesumm" ] ;  then
				return 0;
			else
				return 1;
			fi
			return 0
		else
			return 1;
		fi



}


cd $TEMPROOT && rm -rf "data" || exit 1
tar -xf ${PACKNAME}  || exit 1

index=0
while [ "$index" -lt "$element_count" ]
do
	if [ -s $TEMPDIR/${FILELIST[$index]}  ]; then
#		echo `md5 $TEMPDIR/${FILELIST[$index]}`
	  	equiv_check ${FILELIST[$index]}
	#	echo $? # enabling this will brake if statement below
		if [ "x$?" = x1 ] ; then
			mv -f $DATA/${FILELIST[$index]} $DATA/${FILELIST[$index]}.prev
			cp -f $TEMPDIR/${FILELIST[$index]} $DATA/${FILELIST[$index]}
		else
			echo "$DATA/${FILELIST[$index]} skipped"
		fi

	else
		echo "$TEMPDIR/${FILELIST[$index]} not found or zero size"
	fi

  let "index = $index + 1"
done


#apply whitelist
#fetch -qo /dev/null "http://localhost:13512/admin?action=reloaddata" > /dev/null 2>/dev/null
wget -O /dev/null  -q "http://localhost:13512/admin?action=reloaddata"

#clean

#rm -rf ${PACKNAME}

