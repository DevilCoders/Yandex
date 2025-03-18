#!/bin/sh

SCRIPT_DIR=`dirname $0`
GENERATOR="$SCRIPT_DIR/genaccessip.py"
OUTDIR='/hol/antirobot/data'
OUTFILES='trbosrvnets special_ips yandex_ips'

if  ! $GENERATOR -s $SCRIPT_DIR -o $OUTDIR; then
    echo "Generating special files failed"
    exit 1
fi

for i in $OUTFILES; do
    cp -f ${OUTDIR}/${i} ${OUTDIR}/${i}.prev 2> /dev/null
    [ -e ${OUTDIR}/${i}.new ] && mv ${OUTDIR}/${i}.new ${OUTDIR}/${i}
done

exit 0;
