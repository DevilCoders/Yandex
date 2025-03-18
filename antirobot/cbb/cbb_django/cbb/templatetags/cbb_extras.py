from django import template

register = template.Library()


@register.filter
def get_id(value):
    id = value.split("-")[0]
    if id == "all":
        return None
    else:
        return id


@register.simple_tag
def query_replace(request, field, value):
    """
    Replace a single url parameter (useful for paging)
    """
    dict_ = request.GET.copy()
    dict_[field] = value
    return dict_.urlencode()
