Заходим на страничку: 
https://nanny.yandex-team.ru/ui/#/its/config/edit/locations

Сохраняем содержимое в окне редактирования в файл:
locations-prev.yml

Запускаем:
./genits locations --prev-config-file locations-prev.yml --result-config-file locations-new.yml >locations-mail.txt

https://nanny.yandex-team.ru/ui/#/its/config/edit/ruchkas
Сохраняем содержимое в окне редактирования в файл:
ruchkas-prev.yml

Запускаем:
./genits ruchkas --prev-config-file ruchkas-prev.yml --result-config-file ruchkas-new.yml  >ruchkas-mail.txt

Затем создаем таск в очереди MARTY:
https://st.yandex-team.ru/createTicket?queue=MARTY&_form=23989

Постим содержимое locations-mail.txt в "Конфиг локации", содержимое ruchkas-mail.txt в "Конфиг ручки".



