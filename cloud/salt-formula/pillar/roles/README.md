# Role-specific pillars


**Description:**

Предназначены для переопределения либо установки пилларов, специфичных для конкретной роли (head, compute, billing, etc).

**How to use**

1. Добавить файл с именем `roles/<yourrole>.sls`

**Use cases**

1. nginx
2. push-client
3. logrotate
4. любые другие common формулы, требующие разный набор пилларов для разных ролей.
