# Релизы синтеза

Сервис {{ speechkit-name }} предоставляет обновления в соответствии с системой моделей и версий.

В синтезе речи сервис предоставляет [голоса двух типов](tts/voices.md): стандартные и премиум-голоса. В премиум-голосах используется новая технология синтеза.

Подробнее о голосовых моделях читайте в разделе [О технологии](tts/index.md#voices).

## Текущая версия {#current}

### Релиз 09.06.22 {#090622}

1. Во всех голосах улучшены интонации и акцентирование.

1. Появилось больше возможностей для расстановки пауз: 
   * Исправлена ошибка, из-за которой паузы короче 1200 миллисекунд не учитывались в SSML-разметке. Однако обращаем внимание, что паузы короче 700 миллисекунд считаются подсказкой для синтеза и не позволяют в точности контролировать длительность паузы между словами.
   * Паузы SSML со значениями `x-weak`, `weak`, `medium` имеют большее влияние на синтезируемый текст.
   * Появилась возможность расставлять паузы при использовании TTS-разметки. С помощью тега `<[small]>` можно задавать длительность паузы в синтезируемом тексте, например: `Привет, <[small]>`. Длительность паузы может принимать значения: `tiny`, `small`, `medium`, `large`, `huge`.

1. Закончилась поддержка устаревшей версии голоса `filipp:deprecated`. Теперь `filipp:deprecated` и `filipp` звучат одинаково.

## Предыдущие версии {#previous}

### Релиз 19.05.22 {#190522}

1. С 31 мая 2022 года прекращается поддержка устаревших голосов.

1. В ветке `rc` для тестирования доступны новые голоса и языки: 
   * женский голос `amira` — казахский язык; 
   * мужской голос `john` — английский язык.

   Голоса доступны только в API v3 с использованием заголовка `x-service-branch:rc`.

### Релиз 30.03.22 {#300322}

1. Стандартные голоса теперь доступны только по тегу `:deprecated` и будут поддерживаться до 31 мая 2022 года. 

1. По обращению в техническую поддержку (заявка CLOUDSUPPORT-138703) исправлены интонации и проблемы с редкими артефактами на текстах с большим количеством различных цифр.

### Релиз 17.03.22 {#170322}

1. Появилась возможность синтезировать аудиофайлы в формате MP3. Эта возможность доступна в API v3 и при работе с премиум-голосами через API v1.

1. Для новых голосов появилась поддержка амплуа — расширенной версии эмоциональной окраски (см. параметр `emotion` в [API v1](tts/request.md#body_params) и `role` в [API v3](new-v3/api-ref/grpc/tts_service#Hints)). Разные варианты амплуа доступны для разных голосов. Все значения см. в разделе [{#T}](tts/voices.md). При указании некорректного амплуа сервис вернет ошибку.

1. Исправлена регрессия в качестве расстановки акцентов для голосов `alena` и `filipp`, улучшено качество расстановки акцентов и субъективное восприятие синтеза всех голосов.

1. Начинается большое обновление стандартных голосов: `oksana`, `ermil`, `jane`, `omazh`, `zahar` будут заменены на соответствующие им `oksana:rc`, `ermil:rc`, `jane:rc`, `omazh:rc`, `zahar:rc`. Обновление не затронет стоимость использования обычных голосов. Существующие голоса `oksana`, `ermil`, `jane`, `omazh` и `zahar` доступны в ветке `:deprecated`.

### Релиз 24.01.22 {#240122}

1. Обновлена модель генерации. В новой версии исправлено произношение цифр и аббревиатур сферы финансов.

1. Теперь акценты можно расставлять с помощью выделения: `Вы **рады** меня видеть?`

1. Обработка SSML-пауз и SIL-тегов приведена к единому виду для поддержки интеграции с [Яндекс Диалогами](https://dialogs.yandex.ru). Наличие в тексте пауз в нотации SSML или SIL рассматривается как индикатор конца фразы (utterance) — в генерируемом тексте на месте тега появляется интонация конца фразы. SSML-паузы и SIL-теги поддерживаются при генерации коротких и длинных текстов.

### Релиз 16.12.21 {#161221}

1. Увеличены лимиты для запросов в API v3: длина синтезируемой фразы — 250 символов или 24 секунды аудио. Важно: стоимость запроса пока остается без изменений, но может быть увеличена.

1. Опция `unsafe_mode`, доступная в API v3, позволяет автоматически разделить длинный текст, отправленный для синтеза, на отдельные фразы.

1. Тишина после синтеза последнего слова значительно сократилась. Теперь аудио заканчивается практически сразу после синтеза последнего слова.

### Релиз 18.11.21 {#181121}

1. Внесены исправления, стабилизирующие синтез premium-голоса `alena`. Теперь она звучит однородно.
1. Исправлены ошибки в произношении `alena`.
1. Улучшена расстановка пауз в REST API.
1. В тестовом режиме добавлены новые premium-голоса:
   * `oksana:rc`
   * `ermil:rc`
   * `jane:rc`
   * `omazh:rc`
   * `zahar:rc`

{% if lang == "ru" %}Мы будем рады [отзывам](https://forms.yandex.ru/surveys/10037015.afd97084574df5ac4c7e7199ad406997ac1979e0/) о работе новых premium-голосов!{% endif %}