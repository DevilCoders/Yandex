import datetime
import ipaddress
from django import forms
from django.core.exceptions import ValidationError
from django.utils.translation import ugettext_lazy as _

from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.models import Group


operations = (("add", ""), ("del", ""))
versions = (("4", "IPv4"), ("6", "IPv6"), ("all", "IPv4 and IPv6"))


class MultipleCommaSeparatedField(forms.MultipleChoiceField):
    def to_python(self, value):
        result = []
        if not value:
            return result
        for v in value:
            result.extend([s.strip() for s in v.split(",")])
        return result

    def validate(self, value):
        if self.required and not value:
            raise ValidationError(self.error_messages["required"])


class VersionField(forms.ChoiceField):
    def to_python(self, value):
        value = super(VersionField, self).to_python(value)
        if not value:
            return "4"
        else:
            return value.lower()

    def validate(self, value):
        if value not in ["4", "6", "all"]:
            raise ValidationError("%s is not ip version", value)


class BaseForm(forms.Form):
    version = VersionField(choices=versions, required=False)


class BaseGetForm(BaseForm):
    flag = MultipleCommaSeparatedField(required=True, choices=())

    def clean_flag(self):
        groups_result = list(filter(lambda x: x.isdigit(), self.cleaned_data["flag"]))

        # Повторяем поведение старого api: если во флагах хотя бы одно не-число - нафиг все
        if len(groups_result) != len(self.cleaned_data["flag"]):
            self._errors["flag"] = self.error_class(["Bad flag given"])
        else:
            self.cleaned_data["flag"] = groups_result

        return self.cleaned_data["flag"]


class BaseSetForm(BaseForm):
    flag = forms.IntegerField()
    operation = forms.ChoiceField(choices=operations)
    description = forms.CharField(required=False)

    def clean_flag(self):
        group_id = int(self.cleaned_data["flag"])
        groups_count = db.main.session.query(Group).filter_by(id=group_id, is_active=True).limit(2).count()
        if groups_count == 1:
            return group_id
        else:
            return None


class CheckFlagForm(forms.Form):
    # TODO: get first of get values in query (not last!)
    flag = forms.CharField()

    def clean_flag(self):
        try:
            return [int(i) for i in self.cleaned_data["flag"].split(",")]
        except:
            return []


class GetNetblockForm(BaseGetForm):
    pass


class GetNetexceptForm(BaseGetForm):
    net_ip = forms.GenericIPAddressField(required=False)
    net_mask = forms.IntegerField(required=False, min_value=0, max_value=32)

    def clean(self):
        cleaned_data = super(GetNetexceptForm, self).clean()

        net_ip = cleaned_data.get("net_ip")
        net_mask = cleaned_data.get("net_mask")

        if not net_ip and net_mask:
            msg = "Net ip not set"
            self._errors["net_ip"] = self.error_class([msg])
            self._errors["net_mask"] = self.error_class([msg])
            cleaned_data["net_mask"] = ""
            cleaned_data["net_ip"] = ""

        if net_ip and not net_mask:
            msg = "Net mask not set"
            self._errors["net_ip"] = self.error_class([msg])
            self._errors["net_mask"] = self.error_class([msg])
            cleaned_data["net_mask"] = ""
            cleaned_data["net_ip"] = ""

        return cleaned_data


class GetRangeForm(BaseGetForm):
    formats = [
        ("range_txt", ""),
        ("range_re", ""),
        ("rule_id", ""),
        ("range_src", ""),
        ("range_dst", ""),
        ("flag", ""),
        ("expire", ""),
        ("cdate", ""),
        ("except", ""),
        ("description", ""),
    ]

    flag = MultipleCommaSeparatedField(required=False, choices=())
    range_src = forms.GenericIPAddressField(required=False)
    range_dst = forms.GenericIPAddressField(required=False)
    with_expire = forms.CharField(required=False)
    with_except = forms.CharField(required=False)
    with_format = MultipleCommaSeparatedField(required=False, choices=formats)

    def clean(self):
        if self._errors:
            return self.cleaned_data

        cleaned_data = super(GetRangeForm, self).clean()
        cleaned_data["with_expire"] = (cleaned_data["with_expire"] == "yes")
        cleaned_data["with_except"] = (cleaned_data["with_except"] == "yes")

        range_src = cleaned_data["range_src"]
        range_dst = cleaned_data["range_dst"]
        flag = cleaned_data["flag"]

        if not flag and not (range_src and range_dst):
            msg = "Flag or range_src and range_dst should be in the query"
            self._errors["flag"] = self.error_class([msg])
            self._errors["range_src"] = self.error_class([msg])
            self._errors["range_dst"] = self.error_class([msg])

            del cleaned_data["flag"]
            del cleaned_data["range_src"]
            del cleaned_data["range_dst"]

        elif not range_src and range_dst:
            msg = "Range start not set"
            self.errors["range_src"] = self.error_class([msg])
            self.errors["range_dst"] = self.error_class([msg])
            del cleaned_data["range_src"]
            del cleaned_data["range_dst"]

        elif range_src and not range_dst:
            msg = "Range end not set"
            self.errors["range_src"] = self.error_class([msg])
            self.errors["range_dst"] = self.error_class([msg])
            del cleaned_data["range_src"]
            del cleaned_data["range_dst"]

        return cleaned_data


class SetRangeForm(BaseSetForm):
    range_src = forms.GenericIPAddressField(required=False)
    range_dst = forms.GenericIPAddressField(required=False)
    range_txt = forms.CharField(required=False)
    range_re = forms.CharField(required=False)
    expire = forms.IntegerField(required=False)

    def clean_expire(self):
        expire = self.cleaned_data["expire"]

        # convert to datetime or return None
        if isinstance(expire, int):
            if expire == 0:
                return None
            else:
                expire = datetime.datetime.fromtimestamp(expire)
        elif not isinstance(expire, datetime.datetime):
            return None

        # check if expire date in future
        if expire <= datetime.datetime.now():
            raise ValidationError(_("Invalid date. Expire date must be in the future"))
        return expire

    def clean(self):
        cleaned_data = super(SetRangeForm, self).clean()
        range_src = cleaned_data["range_src"]
        range_dst = cleaned_data["range_dst"]
        range_txt = cleaned_data["range_txt"]
        range_re = cleaned_data["range_re"]

        if sum(1 for x in (range_src or range_dst, range_txt, range_re) if x) != 1:
            raise ValidationError(_("exactly one of range_{src,dst}, range_txt or range_re should be set"))

        if bool(range_src) != bool(range_dst):
            raise ValidationError(_("range_src and range_dst must be either both set or both unset"))

        return cleaned_data


class SetNetblockForm(BaseSetForm):
    net_ip = forms.GenericIPAddressField()
    net_mask = forms.IntegerField()


class SetNetexceptForm(BaseSetForm):
    net_ip = forms.GenericIPAddressField()
    net_mask = forms.IntegerField()
    except_ip = forms.GenericIPAddressField()
    except_mask = forms.IntegerField()


class SetRangesForm(forms.Form):
    flag = forms.IntegerField()
    rng_list = forms.CharField(required=True)
    description = forms.CharField(required=False)

    def clean_flag(self):
        group_id = int(self.cleaned_data["flag"])
        groups_count = db.main.session.query(Group).filter_by(id=group_id, is_active=True).limit(2).count()
        if groups_count == 1:
            return group_id
        else:
            None

    def clean_rng_list(self):
        ips = []
        for line in self.cleaned_data["rng_list"].splitlines():
            try:
                ipaddress.ip_address(line)
                ips += [line]
            except ValueError:
                pass
        if ips:
            return ips
        else:
            return None
