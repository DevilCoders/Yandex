from django.contrib import admin
from core.models import (User, SnippetPool, TaskPool, Snippet, Task,
                         Estimation, QueryBin, TaskPack)

admin.site.register(User)
admin.site.register(QueryBin)
admin.site.register(SnippetPool)
admin.site.register(TaskPool)
admin.site.register(Snippet)
admin.site.register(Task)
admin.site.register(Estimation)
admin.site.register(TaskPack)
