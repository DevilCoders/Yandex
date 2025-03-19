#!/bin/bash

eines="sasta folgaine1 folgaine2 ivaine1 ivaine2 vegaine amseine myteine2 ugreine2"
profiles="corba_lxc-kvm corba_lxc corba_lxc-2hdd"

for i in $eines; do 
    for j in $profiles; do
	echo "$j commiting to $i:"
	eine -s ${i}.yandex-team.ru profile commit $(eine -s ${i}.yandex-team.ru profile create $j rules ${j}.tpl);
    done;
done

rm *~
scp -r * fireball:~/eine
