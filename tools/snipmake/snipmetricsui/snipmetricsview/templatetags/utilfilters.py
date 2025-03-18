from django import template

register = template.Library()

@register.filter(name = 'getItem')
def getItem(value, arg):
    return value[arg]

@register.filter(name = 'getFileName')
def getFileName(value):
    return value.split("/")[-1]

@register.filter(name = 'equals')
def equals(value, arg):
    return value == arg

@register.filter(name = 'getKeys')
def getKeys(value):
    return sorted(value.iterkeys())

@register.filter(name = 'getValue')
def getValue(value, arg):
    return value[arg]

@register.filter(name = 'hasKey')
def hasKey(value, arg):
    return value.has_key(arg)

if __name__ == "__main__":
   register.filter('getItem', getItem)
   register.filter('getFileName', getFileName)
   register.filter('equals', equals)
   register.filter('getValue', getValue)
