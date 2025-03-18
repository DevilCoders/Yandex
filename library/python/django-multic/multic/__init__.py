# coding: utf-8


class DoesNotExistResources(ImportError):
    value = 'Import your resources before import view or form from multic.'

    def __str__(self):
        return str(self.value)

    def __unicode__(self):
        return unicode(self.value)

