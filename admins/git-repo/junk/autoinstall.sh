#!/bin/bash

#executer --cache task list 2>&1| egrep "(TOOLS|CORBA|CALENDAR|CONDUCTOR)" | perl -ne '/locked by inkvizitor68sl/ && print; !/locked/ && print;'

do_testing () {
executer --cache task list 2>&1| egrep "(TOOLS|CORBA)" | grep TESTING | perl -ne '/locked by inkvizitor68sl/ && print; !/locked/ && print;' | awk '{print $1}'
}

do_stable () {
executer --cache task list 2>&1| egrep "(TOOLS|CORBA)" | grep STABLE | perl -ne '/locked by inkvizitor68sl/ && print;
 !/locked/ && print;' | awk '{print $1}'
}

do_task_testing () {
task=$1
#executer 2>&1 --cache tl ${task} | grep -v "Defined alias.*: " | grep -v "Filter: " | grep -v "Executing \`"
executer 2>&1 --cache tl ${task} | grep -v "Defined alias.*: " | grep -v "Filter: " | grep -v "Executing \`" | egrep -v '((CORBA|TOOLS).*\[)' | grep -v Install:  | grep -v ".*=[0-9]" | grep -v ^$ && (echo "Some description, pause now"; return 1; read) || (executer 2>&1 --cache tl ${task} | grep -v "Defined alias.*: " | grep -v "Filter: " | grep -v "Executing \`"; executer 2>&1 --cache do ${task} | grep -v "Defined alias.*: " | grep -v "Filter: ")
}

do_task_stable () {
task=$1
executer 2>&1 --cache tl ${task} | grep -v "Defined alias.*: " | grep -v "Filter: " | grep -v "Executing \`" 
executer 2>&1 --cache do ${task} | grep -v "Defined alias.*: " | grep -v "Filter: "
}


for i in `do_testing`; do do_task_testing $i; done
#for i in `do_stable`; do do_task_testing $i; done
