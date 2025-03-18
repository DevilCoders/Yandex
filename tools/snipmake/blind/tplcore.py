def quote(text):
    return text.replace("&", "&amp;").replace("\"", "&quot;").replace("<", "&lt;").replace(">", "&gt;")

def returnBold(text):
    return text.replace("&lt;b&gt;", "<b>").replace("&lt;/b&gt;", "</b>")

def render(template, values = {}):
    result = template
    for k, v in values.iteritems():
        result = result.replace(u"$$" + unicode(k) + u"$$", returnBold(quote(unicode(v))))
    return result

