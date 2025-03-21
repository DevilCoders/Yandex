# Как отредактировать задачу

Чтобы изменить задачу, перейдите на ее страницу. Если у вас не хватает прав для редактирования, [запросите их](../faq.md#section_xgr_zng_4bb) у владельца очереди {% if audience == "external" %} или [администратора](../role-model.md) вашей организации{% endif %}.

{% note info %}

Страницы задач обновляются в реальном времени. Если кто-то изменит или прокомментирует задачу, страница которой открыта, вы увидите уведомление об этом.

{% endnote %}

## Изменить название задачи {#section_name}

{% list tabs %}

- Веб-интерфейс

  Справа от названия задачи нажмите значок ![](../../_assets/tracker/icon-edit.png). Отредактируйте текст и нажмите значок ![](../../_assets/tracker/approve-checkmark.png) или клавишу Enter.

- Мобильное приложение

  Нажмите на название задачи. Отредактируйте текст и нажмите кнопку **Сохранить**.

{% endlist %}

## Изменить описание задачи {#section_yrw_npn_jz}

{% list tabs %}

- Веб-интерфейс

  Справа от описания задачи нажмите значок ![](../../_assets/tracker/icon-edit.png) и отредактируйте текст. Для форматирования текста используйте [разметку YFM](markup.md). Чтобы сохранить изменения, нажмите кнопку **Сохранить**.

  Если вы не сохранили описание задачи и закрыли либо перезагрузили страницу, ваше описание сохранится в черновиках. Чтобы восстановить текст из черновика, снова нажмите значок редактирования ![](../../_assets/tracker/icon-edit.png), затем на панели инструментов нажмите значок ![](../../_assets/tracker/drafts.png) и выберите черновик.

  В описании задачи можно указать ключ любой другой задачи — тогда {{ tracker-name }} автоматически их свяжет.

  {% if audience == "internal" %}
  {% note info %}

  Если в тексте вам необходимо вставить ссылку на другую задачу, но при этом избежать связывания, перед ключом задачи укажите `st:` (например, `st:TEST-1234`). В таком случае в тексте появится [магическая ссылка](wiki.md#magic-link-descr), но связь между задачами создана не будет.
  Полностью отключить автоматическое связывание задач из разных очередей можно в [настройках очереди](../manager/edit-queue-general.md).

  {% endnote %}
  {% endif %}

- Мобильное приложение

  Справа от описания задачи нажмите значок ![](../../_assets/tracker/icon-edit.png) и отредактируйте текст. Для форматирования текста используйте [разметку YFM](markup.md). Чтобы сохранить изменения, нажмите кнопку **Сохранить**.

  В описании задачи можно указать ключ любой другой задачи — тогда {{ tracker-name }} автоматически их свяжет. Связанная задача отобразится на вкладке **Связи**. 

  {% if audience == "internal" %}
  {% note info %}

   Чтобы добавить ссылку на другую задачу без создания связи с ней, воспользуйтесь веб-интерфейсом.

  {% endnote %}
  {% endif %} 

{% endlist %}

В описании задачи также можно [прикрепить изображение или файл](attach-file.md).  


## Изменить параметры задачи {#section_jqw_ppn_jz}

{% list tabs %}

- Веб-интерфейс

  [Параметры задачи](create-param.md#section_ymd_ycj_1gb) отображаются на панели справа. Чтобы изменить значение параметра, нажмите на его название. Чтобы сохранить изменения, нажмите кнопку **ОК**.

  Если вы не видите на панели справа нужных параметров, добавьте их с помощью кнопки ![](../../_assets/tracker/task-params-btn.png) **Выбрать поля**.

- Мобильное приложение

  [Параметры задачи](create-param.md#section_ymd_ycj_1gb) отображаются на вкладке **Параметры**. Чтобы изменить значение параметра, нажмите на значок ![](../../_assets/tracker/mobile-params-open.png). Введите значение и нажмите кнопку **Сохранить**.

  {% note info %}
  
  Если вы не видите нужных параметров, перейдите в веб-приложение и на панели справа добавьте их с помощью кнопки ![](../../_assets/tracker/task-params-btn.png) **Выбрать поля**. 

  {% endnote %}

  Параметры **Приоритет**, **Тип**, **Исполнитель** и **Дедлайн** также расположены на вкладке **Описание**. Чтобы отредактировать их значения, нажмите на название параметра, укажите значение и нажмите кнопку **Сохранить**. 
  

{% endlist %}

## Изменить статус задачи {#section_status}

{% list tabs %}

- Веб-интерфейс

  Чтобы изменить статус задачи, под названием задачи слева от кнопки **Действия** нажмите на кнопку с нужным названием статуса. 

  
- Мобильное приложение

  Чтобы изменить статус задачи, под названием задачи откройте выпадающий список и выберите статус. 

{% endlist %}  


## Настроить отображение задачи {#section_display}

{% list tabs %}

- Веб-интерфейс

  Чтобы вам было удобнее работать в {{ tracker-name }}, настройте внешний вид страницы задачи: язык интерфейса, отображение пользователей и дат, порядок комментариев. Для этого перейдите в раздел [**Персональные настройки**](personal.md). 

  Настройки, которые установлены для веб-интерфейса, автоматически применяются для мобильного приложения. 

  
- Мобильное приложение

  Чтобы вам было удобнее работать в {{ tracker-name }}, настройте внешний вид страницы задачи: язык интерфейса, отображение пользователей и дат, порядок комментариев. Для этого перейдите в веб-интерфейс в раздел [**Персональные настройки**](personal.md).


{% endlist %}  



## Другие действия над задачами  

В задачи можно добавлять комментарии, связи, файлы и выполнять другие действия:

* [{#T}](comments.md)
* [{#T}](checklist.md)
* [{#T}](ticket-links.md)
* [{#T}](attach-file.md)
* [{#T}](move-ticket.md)

