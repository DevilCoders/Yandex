# -*- coding: utf-8 -*-

from django import template


register = template.Library()


@register.tag(name='popup')
def do_popup(parser, token):
    try:
        (tag, popup_info_struct) = token.split_contents()
        nodelist = parser.parse(('endpopup',))
        parser.delete_first_token()
    except ValueError:
        msg = 'popup tag requires popup_info_struct as argument'
        raise template.TemplateSyntaxError(msg)
    return Popup(popup_info_struct, nodelist)


class Popup(template.Node):
    info_struct = None
    nodelist = ''

    def __init__(self, popup_info_struct, nodelist):
        self.info_struct = popup_info_struct
        self.nodelist = nodelist

    def render(self, context):
        inner_html = self.nodelist.render(context)
        try:
            info_struct = template.Variable(self.info_struct).resolve(context)
            csrf_token = context.get('csrf_token', '')
        except template.VariableDoesNotExist:
            info_struct = ''
            csrf_token = ''
        tpl = template.loader.get_template('core/popup.html')
        return tpl.render(template.Context({'inner_html': inner_html,
                                            'info_struct': info_struct,
                                            'csrf_token': csrf_token},
                                           autoescape=context.autoescape))
