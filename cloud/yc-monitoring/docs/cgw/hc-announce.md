# hc-announce

**Значение:** наличие анонса default'ного маршрута в сеть для hc-nod
**Воздействие:** если горит на всех lb-nod'ах в зоне, перестанут корректно работать hc-nod'ы
**Что делать:** см. проблемы в drain-status
Либо проблема при работе сервиса yalb-init-vpp для для преднастройки lb-nod'ы, рестарт vpp
