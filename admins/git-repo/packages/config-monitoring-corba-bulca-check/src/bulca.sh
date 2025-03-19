#!/bin/bash
rank=`cat /etc/baida/baida.conf | grep rank | tr '<' ' ' | tr '>' ' ' | awk '{print $2}'`
answer=$(curl --max-time 3 http://localhost:10000/status/ -w "code: %{http_code}" -o /dev/null 2>/dev/null)
slave=`cat /etc/baida/baida.conf | grep slave_url | tr '<' ' ' | tr '>' ' ' | awk '{print $2}'`
slave_answer=$(curl --max-time 3 $slave/status/ -w "code: %{http_code}" -o /dev/null 2>/dev/null)

if [[ $answer != "code: 200" ]]
        then echo "PASSIVE-CHECK:bulca;2; $answer"
else
	if [[ "$rank" == "master" ]]; then
		if [[ "$slave_answer" != "code: 200" ]]; then
			echo "PASSIVE-CHECK:bulca;2; $rank ok,but its slave $slave $slave_answer . Need R\O"
		fi	
        fi
	echo "PASSIVE-CHECK:bulca;0;ok,$rank"
fi
