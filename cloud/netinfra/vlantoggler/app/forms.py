#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField, RadioField
from wtforms.validators import DataRequired


class VlanToggler(FlaskForm):
    tor = StringField('ToR name', validators=[DataRequired()])
    interface = StringField('Interface name', validators=[DataRequired()])
    state = RadioField('Desired state', choices=[('setup', 'setup'), ('prod', 'prod')], validators=[DataRequired()])
    submit = SubmitField('Switch')
