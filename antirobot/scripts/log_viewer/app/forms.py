from flask_wtf import FlaskForm

from wtforms import DateField, TextField, BooleanField, SubmitField, HiddenField, IntegerField, StringField, TextAreaField
from wtforms.validators import Required, IPAddress, Optional, InputRequired

def StripSpaces(value):
    if value is not None and hasattr(value, 'strip'):
        return value.strip()

    return value


StdFilters = [StripSpaces]

class StrippedDateField(DateField):
    def process_formdata(self, valuelist):
        stripped = [valuelist[0].strip()] if valuelist else []
        super(StrippedDateField, self).process_formdata(stripped)


class BaseQueryForm(FlaskForm):
    date = StrippedDateField(label='Date', format='%Y%m%d', validators=[Required()], description='YYYYMMDD, e.g. 20160714')
    ip = TextField(label='Ip', validators=[IPAddress(ipv6=True), Optional()], filters=StdFilters)
    yandexuid = TextField(label='Yandexuid', filters=StdFilters)
    slow = BooleanField(label='allow slow search')
    doreq = HiddenField(default='1')


class AccesslogQueryForm(BaseQueryForm):
    class Meta:
        csrf = False

    substring = TextField(label='Substring', description='substring in  access_log (yandexuid, login etc.', filters=StdFilters)
    isre = BooleanField('isre', description='substring is regexp')
    shortlog = BooleanField(label='reduced log')


class EventlogQueryForm(BaseQueryForm):
    class Meta:
        csrf = False

    token = TextField(label='Token', description='filter by token')
    substring = TextField(label='Substring', description="in request's body")


class Ip2BackendForm(FlaskForm):
    class Meta:
        csrf = False

    ip = StringField('IP', validators=[InputRequired(), IPAddress(ipv6=True)])
    options = StringField('Options', validators=[Optional()])


class CheckAntiDDosRuleForm(FlaskForm):
    rules = TextAreaField('Rules', validators=[InputRequired()])
