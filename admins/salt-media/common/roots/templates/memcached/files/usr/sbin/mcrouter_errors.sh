#!/bin/bash
{% set port = salt['pillar.get']('memcached:mcrouter:port', 5000) -%}
{% set err_limit = salt['pillar.get']('memcached:mcrouter:err_limit', 1) -%}

stat_file='/root/mcrouter_errors.stat'

# error limit in percents, integer value
limit={{err_limit}}

if [[ -r "$stat_file" ]]; then
    prev_errors=$(cat ${stat_file} | grep errors)
    prev_errors_1=$(echo ${prev_errors} | perl -naE'if(scalar@F>=1){say$F[1]}else{say"0"}')
    prev_errors_2=$(echo ${prev_errors} | perl -naE'if(scalar@F>=2){say$F[2]}else{say"0"}')
    prev_errors_3=$(echo ${prev_errors} | perl -naE'if(scalar@F>=3){say$F[3]}else{say"0"}')

    prev_gets=$(cat ${stat_file} | grep gets)
    prev_gets_1=$(echo ${prev_gets} | perl -naE'if(scalar@F>=1){say$F[1]}else{say"0"}')
    prev_gets_2=$(echo ${prev_gets} | perl -naE'if(scalar@F>=2){say$F[2]}else{say"0"}')
    prev_gets_3=$(echo ${prev_gets} | perl -naE'if(scalar@F>=3){say$F[3]}else{say"0"}')
else
    prev_errors_1=0;
    prev_errors_2=0;
    prev_errors_3=0;

    prev_gets_1=0;
    prev_gets_2=0;
    prev_gets_3=0;
fi

curr_errors=$(echo 'stats all' | nc -w 5 localhost {{port}} | perl -naE'if(/result_error_all_count/){print $F[2]}')
curr_gets=$(echo 'stats all' | nc -w 5 localhost {{port}} | perl -naE'if(/cmd_get_count/){print $F[2]}')

echo "errors ${curr_errors} ${prev_errors_1} ${prev_errors_2}" > ${stat_file}
echo "gets ${curr_gets} ${prev_gets_1} ${prev_gets_2}" >> ${stat_file}

if [[ $((${curr_gets} - ${prev_gets_1})) -gt 0 ]]; then
    diff_1=$(((${curr_errors} - ${prev_errors_1}) * 100 / (${curr_gets} - ${prev_gets_1})))
else
    diff_1=0;
fi

if [[ $((${prev_gets_1} - ${prev_gets_2})) -gt 0 ]]; then
    diff_2=$(((${prev_errors_1} - ${prev_errors_2}) * 100 / (${prev_gets_1} - ${prev_gets_2})))
else
    diff_2=0;
fi

if [[ $((${prev_gets_2} - ${prev_gets_3})) -gt 0 ]]; then
    diff_3=$(((${prev_errors_2} - ${prev_errors_3}) * 100 / (${prev_gets_2} - ${prev_gets_3})))
else
    diff_3=0;
fi

if [[ "$diff_1" -gt ${limit} && "$diff_2" -gt ${limit} && "$diff_3" -gt ${limit} ]]; then
    status='2;'
elif [[ "$diff_1" -gt ${limit} || "$diff_2" -gt ${limit} || "$diff_3" -gt ${limit} ]]; then
    status='1;'
else
    status='0;OK! '
fi

echo "${status}errors for last 3 checks: ${diff_1}%, ${diff_2}%, ${diff_3}%"
