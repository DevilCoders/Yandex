#!/bin/sh

MACRO='_TRBOSRVNETS_'

FETCH='/usr/bin/fetch'
OUTFILE='/hol/antirobot/data/trbosrvnets'

#FETCH -q -o - https://racktables.yandex.net/export/expand-fw-macro.php?macro=${MACRO}

if [ -f ${OUTFILE} ] ; then

	${FETCH} -q -o - https://racktables.yandex.net/export/expand-fw-macro.php?macro=${MACRO} > ${OUTFILE}.new

	if [ -s ${OUTFILE}.new ] ; then
		cp -f ${OUTFILE} ${OUTFILE}.prev
        mv  ${OUTFILE}.new  ${OUTFILE}
	else
		echo "no _TRBOSRVNETS_  recieved"
#       exit 1;
	fi

else

	${FETCH} -q -o - https://racktables.yandex.net/export/expand-fw-macro.php?macro=${MACRO} > ${OUTFILE}

fi


SP_OUT='/hol/antirobot/data/special_ips'
SP_MACRO='_SEARCHQUALITYRATING_'
#TEMPFILE=`/usr/bin/mktemp /var/tmp/special_ips.XXXXXXX`



if [ -f ${SP_OUT} ] ; then

		${FETCH} -q -o - https://racktables.yandex.net/export/expand-fw-macro.php?macro=${SP_MACRO} > ${SP_OUT}.new
	
    if [ -s ${SP_OUT}.new ] ; then
        cp -f ${SP_OUT} ${SP_OUT}.prev
        mv  ${SP_OUT}.new  ${SP_OUT}
    else
        echo "no _SEARCHQUALITYRATINGDOWNLOADERS_  recieved"
#       exit 1;
    fi

else
		${FETCH} -q -o - https://racktables.yandex.net/export/expand-fw-macro.php?macro=${SP_MACRO} > ${SP_OUT}
fi


exit 0;
