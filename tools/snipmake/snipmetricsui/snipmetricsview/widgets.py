#!/usr/bin/python
# -*- coding: utf-8 -*-

from django import forms
from django.forms import widgets
from django.db import models

class CheckboxTreeField(forms.MultipleChoiceField):

    def valid_value(self, value):
        return self.valid_value_insubtree(self.choices, value)

    def valid_value_insubtree(self, subtree, value):
        for choice in subtree:
            if isinstance(choice, list):
                if self.valid_value_insubtree(choice, value):
                    return True
            else:
                if choice[0] == value:
                    return True
        return False


class CheckboxTreeWidget(widgets.Select):

    def __init__(self, attrs = None, choices = ()):
        self.attrs = attrs or {}
        self.choices = list(choices)

    def buildTree(self, name, values, ulid=""):
        res = u"<ul class='unorderedlisttree' "
        if ulid != "":
           res += u"id='" + ulid + u"'"
        res += u">"
        for val in values:
           item = val[0] if isinstance(val, list) else val
           checked = ""
           if item[2]:
              checked = "checked"
           res += u"<li><input type='checkbox' name='" + name + u"' id='" + item[0] + u"' value='" + item[0] + u"'" + checked + u" /><label for='" + item[0] +u"' class='" + checked +"'>" + item[1] + u"</label>"
           if isinstance(val, list):
               res += self.buildTree(name, val[1:])
           res += u"</li>"
        res += u"</ul>"
        return res

    def getJsCode(self, name, values):
        res = '''<script>
               jQuery(document).ready(function(){
                 jQuery("#''' + name + '''").checkboxTree(
                {
               collapsedarrow: "img/img-arrow-collapsed.gif",
               expandedarrow: "img/img-arrow-expanded.gif",
               blankarrow: "img/img-arrow-blank.gif",
               collapsed : true,
               checkchildren: true
            });
              })
         </script>'''
        return res

    def render(self, name, value, attrs = None, choices = ()):
       if choices != ():
          self.choices = choices
       return str(self.media) + self.getJsCode(name, self.choices) + self.buildTree(name, self.choices, name)

    class Media:
        js = ('scripts/jquery.checkboxtree.js', )
        css = { 'all' : ('css/checkboxtree.css',) }

class CheckboxTriggerWidget(widgets.CheckboxInput):
    def __init__(self, attrs = {}, dependendField = ''):
        self.attrs = attrs
        self.dependendField = dependendField

    def render(self, name, value, attrs = {}, choices = ()):
        return str(self.media) + "<input type='checkbox' id ='" + name + "' name='" + name + "' onclick='switchFieldVisibility(\"" + self.dependendField + "\")' " + (" checked " if value else "") + "/>"
        
    class Media:
        js = ('scripts/basescripts.js', )
        
