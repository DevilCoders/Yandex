import django.template

register = django.template.Library()


@register.filter
def has_perm(user, permission):
    return user.has_perm(permission)
