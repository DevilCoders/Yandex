#!/bin/bash
export TMPDIR=/tmp/`whoami`
if [ ! -d $TMPDIR ]; then
    mkdir $TMPDIR
fi;
python manage.py harvest -S core/features/developer.feature
python manage.py harvest -S core/features/assessor.feature
python manage.py harvest -S core/features/aadmin.feature


python manage.py harvest -S core/features/parallel_changes.feature

rm $TMPDIR/lettuce-django.pid
