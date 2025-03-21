# Изменить связи задачи

Задачу можно связать с другими задачами, чтобы сгруппировать задачи с общей темой или обозначить их зависимость друг от друга.

Число связей задачи не ограничено. Список связанных задач отображается под описанием задачи.

Также к задаче можно привязать [коммит в репозиторий](#section_commit). 

{% note info %}

Связь с задачей создается автоматически, если указать ее ключ в комментарии или описании.

{% endnote %}

## Добавить связь {#add-link}

{% list tabs %}

- Веб-интерфейс

    Чтобы создать связь с другой задачей:

    1. Выберите **Действия** → **Добавить связь**.

    1. Выберите подходящий [тип связи](links.md).

    1. Укажите ключ задачи, с которой нужно создать связь. Найти ключ можно на странице задачи сразу под заголовком (например, `TEST-1234`).

    1. Нажмите **Добавить связь**.

- Мобильное приложение
    
    Чтобы создать связь с другой задачей:
    
    1. Вставьте ключ задачи в комментарий или описание. Связанная задача отобразится на вкладке **Связи**.
    
    1. При необходимости [измените тип связи](#change-link-type) на любой из доступных.

{% endlist %}

## Создать подзадачу {#create-subtask}

Сложные задачи можно разбить на более простые подзадачи и отслеживать их выполнение отдельно. После создания подзадачи вы можете изменить [тип связи](links.md).

{% list tabs %}

- Веб-интерфейс

    Чтобы создать подзадачу:

    1. Откройте страницу задачи, к которой вы хотите создать подзадачу.

    1. Выберите **Действия** → **Создать подзадачу**.

    1. Заполните поля так же, как при [создании новой задачи](#create-task).

    1. Нажмите кнопку **Создать**. Рядом с названием подзадачи отобразится ссылка на родительскую задачу. 

- Мобильное приложение

    Чтобы создать подзадачу:

    1. Откройте задачу, к которой вы хотите создать подзадачу.

    1. В правом верхнем углу экрана нажмите ![](../../_assets/tracker/dots.png) и выберите **Создать подзадачу**.

    1. Заполните поля так же, как при [создании новой задачи](#create-task).

    1. Нажмите кнопку **Создать задачу**. Рядом с названием подзадачи отобразится ссылка на родительскую задачу. 


{% endlist %}

## Изменить тип связи {#change-link-type}

{% list tabs %}

- Веб-интерфейс

    Чтобы изменить [тип связи](links.md) у задачи:

    1. Откройте страницу одной из двух связанных задач.

    1. В списке под описанием задачи выберите связь, тип которой требуется изменить.

    1. Напротив связанной задачи нажмите кнопку ![](../../_assets/tracker/link.png) и выберите новый тип.

- Мобильное приложение

    Чтобы изменить [тип связи](links.md) у задачи:

    1. Откройте одну из двух связанных задач.

    1. Перейдите на вкладку **Связи**.

    1. Напротив связанной задачи нажмите кнопку ![](../../_assets/tracker/dots.png) → **Изменить тип связи** и выберите новый тип.

{% endlist %}

## Удалить связь {#delete-link}

{% list tabs %}

- Веб-интерфейс

    Чтобы удалить связь у задачи:

    1. Откройте страницу одной из двух связанных задач.

    1. В списке под описанием задачи выберите связь, которую требуется удалить.

    1. Напротив связанной задачи нажмите кнопку ![](../../_assets/tracker/remove-task-type.png).

- Мобильное приложение

    Чтобы удалить связь у задачи:

    1. Откройте одну из двух связанных задач.

    1. Перейдите на вкладку **Связи**.

    1. Напротив связанной задачи нажмите кнопку ![](../../_assets/tracker/dots.png) и выберите **Удалить связь**.

{% endlist %}

## Сделать задачу подзадачей {#make-subtask}

Задачу можно сделать частью более крупной (родительской) задачи:

1. Выберите **Действия** → **Сделать подзадачей**.

1. Укажите ключ родительской задачи. Найти ключ можно на странице задачи сразу под заголовком (например, `TEST-1234`).

1. Нажмите **Сохранить**.

## Изменить родительскую задачу {#edit-parent-task}

1. Откройте страницу подзадачи.

1. Нажмите на значок ![](../../_assets/tracker/edit-parent.png) возле названия родительской задачи в верхней части страницы.

1. Введите ключ задачи, которую вы хотите сделать родительской.

1. Нажмите кнопку **Сохранить**.

## Удалить связь с родительской задачей {#delete-parent-task}

Чтобы удалить связь с родительской задачей:

1. Откройте страницу дочерней задачи.

1. Нажмите на значок ![](../../_assets/tracker/edit-parent.png) возле названия родительской задачи в верхней части страницы.

1. Нажмите кнопку **Удалить связь**.

## Связать коммит с задачей {#section_commit}

Вы можете привязать к задаче коммит в репозиторий{% if audience == "external" %}, который [подключен к {{ tracker-name }}](../manager/add-repository.md){% endif %}. Для этого укажите ключ задачи в комментарии к коммиту. Привязанные коммиты можно просмотреть:

- на странице задачи на вкладке **Коммиты**;
- на странице очереди на вкладке **Коммиты**.

Если вы не видите вкладки **Коммиты**, убедитесь, что она включена в [настройках очереди](../manager/edit-queue-general.md).

