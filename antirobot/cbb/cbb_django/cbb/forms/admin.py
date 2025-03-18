from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.library.common import compare_ips
from antirobot.cbb.cbb_django.cbb.library.data import get_exceptions_for_net
from antirobot.cbb.cbb_django.cbb.library.errors import CbbError
from antirobot.cbb.cbb_django.cbb.library.fields import (ExpireField, ExpirePeriodField, GenericIPAddressFieldRus)
from antirobot.cbb.cbb_django.cbb.models import Group
from django import forms
from ipaddr import IPAddress, IPNetwork

group_types = (
    ("0", "0 - Произвольные промежутки без expire"),
    ("1", "1 - Одиночные ip с expire"),
    ("2", "2 - Сети и исключения"),
    ("3", "3 - Текстовые блокировки"),
    ("4", "4 - Список регулярок"),
)


class ChangeGroupInfoForm(forms.Form):
    name = forms.RegexField(widget=forms.TextInput(attrs={"class": "form-control", "placeholder": "Название"}), regex=r"^[a-z0-9_-]+$")
    group_descr = forms.CharField(widget=forms.TextInput(attrs={"class": "form-control", "placeholder": "Описание", "maxlength": 200}))
    is_internal = forms.BooleanField(required=False)
    default_type = forms.ChoiceField(choices=group_types, initial="0", widget=forms.RadioSelect())
    group_id = forms.IntegerField(widget=forms.HiddenInput(), required=False, initial=None)
    period_to_expire = ExpirePeriodField(widget=forms.TextInput(attrs={"class": "form-control", "placeholder": "Например 42d 7h 0m"}), required=False)
    responsible_people = forms.CharField(widget=forms.HiddenInput(), required=True)

    def clean(self):
        name = self.cleaned_data.get("name")
        if name is None:
            return self.cleaned_data
        group_id = self.cleaned_data.get("group_id")
        group = Group.query.filter_by(name=name, is_active=True).all()
        if group and group_id != group[0].id:
            self._errors["name"] = self.error_class(["Группа с таким именем уже существует."])
        return self.cleaned_data

    def make_readonly(self):
        for field in self.fields.values():
            field.widget.attrs["readonly"] = True


class AddServiceForm(forms.Form):
    name = forms.RegexField(widget=forms.TextInput(attrs={"class": "form-control", "placeholder": "Название"}), regex=r"^[a-z0-9_-]+$")
    responsible_people = forms.CharField(widget=forms.HiddenInput(), required=True)


class FindBlockForm(forms.Form):
    search = forms.CharField()

    def clean_search(self):
        search = self.cleaned_data.get("search")
        if search:
            search = search.strip()
        return search

    def clean(self):
        search = self.cleaned_data.get("search")
        if search is None:
            return self.cleaned_data
        try:
            self.cleaned_data["version"] = IPAddress(search).version
        except ValueError:
            self.cleaned_data["version"] = "txt"
        return self.cleaned_data


class AddRangeForm(forms.Form):
    ip_version = forms.IntegerField(required=False, initial=4, widget=forms.HiddenInput())
    block_descr = forms.CharField(required=False, widget=forms.Textarea(attrs={"class": "form-control", "placeholder": "Почему блокируем?", "rows": 3}))
    rng_list = forms.CharField(required=False, widget=forms.Textarea(attrs={"class": "form-control", "placeholder": "Блокировки", "rows": 4}))


class AddRangeIpForm(AddRangeForm):
    rng_start = GenericIPAddressFieldRus(required=False, widget=forms.TextInput(attrs={"placeholder": "Начало промежутка", "class": "form-control"}))
    rng_end = GenericIPAddressFieldRus(required=False, widget=forms.TextInput(attrs={"placeholder": "Конец промежутка", "class": "form-control"}))

    def clean(self):
        if not self._errors:
            rng_start = self.cleaned_data.get("rng_start")
            rng_end = self.cleaned_data.get("rng_end")
            rng_list = self.cleaned_data.get("rng_list")

            if rng_start:
                if not rng_end:
                    self.cleaned_data["rng_end"] = rng_start
                    rng_end = rng_start
                try:
                    version = compare_ips(rng_start, rng_end, True)
                except CbbError as e:
                    self._errors["rng_end"] = self.error_class([str(e)])
                    return self.cleaned_data
                except Exception:
                    self._errors["rng_end"] = self.error_class(["Введите верный ipv4 или ipv6 адрес."])
                    return self.cleaned_data
                self.cleaned_data["ip_version"] = version

            if rng_list:
                rng_list_result = []
                for rng in rng_list.split("\r\n"):
                    if not rng:
                        continue
                    rng = rng.split()
                    if len(rng) > 2:
                        self._errors["rng_list"] = self.error_class(["Неверный формат: %s" % " ".join(rng)])
                        continue
                    try:
                        if len(rng) == 2:
                            version = compare_ips(rng[0], rng[1], True)
                            rng_list_result.append((rng[0], rng[1], version))
                        elif len(rng) == 1:
                            if "/" in rng[0]:
                                net = IPNetwork(rng[0])
                                rng_list_result.append((str(net.network), str(net.broadcast), net.version))
                            else:
                                rng_start = IPAddress(rng[0])
                                rng_list_result.append((rng[0], rng[0], rng_start.version))
                    except Exception as e:
                        if isinstance(e, CbbError):
                            msg = "%s: %s" % (str(e), " ".join(rng))
                        else:
                            msg = "Введите верный ipv4 или ipv6 адрес: %s" % " ".join(rng)

                        if self._errors.get("rng_list") is None:
                            self._errors["rng_list"] = self.error_class([msg])
                        else:
                            self._errors["rng_list"].append(msg)
                self.cleaned_data["rng_list"] = rng_list_result
            if not rng_list and not rng_start:
                self._errors["rng_start"] = self.error_class(["Не указано начало промежутка."])

        return self.cleaned_data


class AddSingleIpForm(AddRangeIpForm):
    expire = ExpireField(required=False)


class AddNetForm(AddRangeForm):
    net_ip = GenericIPAddressFieldRus(required=False, widget=forms.TextInput(attrs={"placeholder": "IP-адрес", "class": "form-control"}))
    net_mask = forms.IntegerField(required=False, widget=forms.TextInput(attrs={"class": "form-control", "placeholder": "Маска", }))

    def clean(self):
        if not self._errors:
            net_ip = self.cleaned_data.get("net_ip")
            net_mask = self.cleaned_data.get("net_mask")
            rng_list = self.cleaned_data.get("rng_list")
            if net_ip and net_mask:
                try:
                    net = IPNetwork(net_ip + "/" + str(net_mask))
                except Exception:
                    self._errors["net_mask"] = self.error_class(["Ошибка в формате сети"])
                    return self.cleaned_data
                self.cleaned_data["ip_version"] = net.version
            elif rng_list:
                net_result = []
                for rng in rng_list.split("\r\n"):
                    if not rng:
                        continue
                    try:
                        net = IPNetwork(rng)
                        net_result.append((str(net.network), str(net.prefixlen), net.version))
                    except Exception as e:
                        if isinstance(e, CbbError):
                            msg = "%s: %s" % (str(e), rng)
                        else:
                            msg = "Ошибка в формате сети: %s" % rng
                        if self._errors.get("rng_list") is None:
                            self._errors["rng_list"] = self.error_class([msg])
                        else:
                            self._errors["rng_list"].append(msg)
                self.cleaned_data["rng_list"] = net_result
            elif not net_ip:
                self._errors["net_ip"] = self.error_class(["IP-адрес сети не указан."])
            elif not net_mask:
                self._errors["net_mask"] = self.error_class(["Маска сети не указана."])
        return self.cleaned_data


class AddBlockTxtForm(AddRangeForm):
    expire = ExpireField(required=False)
    rng_txt = forms.CharField(widget=forms.Textarea(attrs={
        "class": "form-control",
        "rows": 3,
        "placeholder": "Текст",
        "default": "ddd",
    }))


class AddBlockReForm(AddRangeForm):
    expire = ExpireField(required=False)
    rng_re = forms.CharField(widget=forms.Textarea(attrs={
        "class": "form-control",
        "rows": 3,
        "placeholder": "Текст",
        "default": "ddd",
    }))


def get_same_service_groups(group_id=None, default_types=("3")):
    """
    Returns groups' ids that belongs to same service as given group
    """
    if group_id is None:
        return
    group_descr = Group.query.filter_by(id=group_id).one().group_descr
    services = [service for service in group_descr.split(' ') if len(service) > 0 and service[0] == '#']
    group_to_choose = []
    if (len(services) == 0):
        group_to_choose = [group_id]
    for service in services:
        group_to_choose += Group.get_service_active_groups(service=service, default_types=default_types)
    uniq_group_id = set(group_to_choose)
    return uniq_group_id


class EditRangeForm(forms.Form):

    new_group = forms.ChoiceField(required=False)
    bin_choices = (
        ('-2', "Без изменений"),
        ('-1', "Выкатить на 100%"),
        ('0', "Бин 0"),
        ('1', "Бин 1"),
        ('2', "Бин 2"),
        ('3', "Бин 3"),
    )
    bin_choices_for_services = bin_choices[:3]

    new_expire = ExpireField(required=False)
    exp_bin = forms.ChoiceField(required=False, choices=bin_choices)
    exp_bin_for_services = forms.ChoiceField(required=False, choices=bin_choices_for_services)
    move_descr = forms.CharField(required=False, widget=forms.Textarea(attrs={"class": "form-control", "rows": 3, "placeholder": "Почему перемещаем?"}))

    ip_version = forms.ChoiceField(choices=(("4", "4"), ("6", "6")),
                                   initial="4", required=False,
                                   widget=forms.HiddenInput())
    rng_start = GenericIPAddressFieldRus(required=False, widget=forms.HiddenInput())
    rng_end = GenericIPAddressFieldRus(required=False, widget=forms.HiddenInput())
    rng_txt = forms.IntegerField(required=False, widget=forms.HiddenInput())
    rng_re = forms.IntegerField(required=False, widget=forms.HiddenInput())
    operation_descr = forms.CharField(required=False, widget=forms.Textarea(attrs={"class": "form-control", "rows": 3, "placeholder": "Почему разблокируем?"}))

    @db.main.use_slave()
    def __init__(self, data=None, initial=None, group=None, *args, **kwargs):
        super(EditRangeForm, self).__init__(data, initial, *args, **kwargs)
        self.group = group
        group_choices = [(group.id, "Без изменений")]
        for gr_id in get_same_service_groups(group_id=group.id, default_types=["3"]):
            gr = Group.query.filter_by(id=gr_id).one()
            group_choices.append((gr.id, str(gr.id) + " - " + gr.group_descr))
        self.fields["new_group"].choices = group_choices

    def clean(self):
        rng_start = self.cleaned_data.get("rng_start")
        rng_end = self.cleaned_data.get("rng_end")
        rng_txt = self.cleaned_data.get("rng_txt")
        rng_re = self.cleaned_data.get("rng_re")

        if not any([rng_start, rng_end, rng_txt, rng_re]):
            self._errors["__all__"] = self.error_class(["Недостаточно параметров: обязательны rng_start, rng_end, rng_txt либо rng_re"])
            return self.cleaned_data

        if sum(1 for x in (rng_start or rng_end, rng_txt, rng_re) if x) > 1:
            self._errors["__all__"] = self.error_class(["Слишком много параметров: rng_start, rng_end, rng_txt либо rng_re"])

        if rng_txt and self.group.default_type != "3":
            self._errors["rng_txt"] = self.error_class(["Неверный параметр для группы с нетекстовыми блокировками"])

        if rng_re and self.group.default_type != "4":
            self._errors["rng_re"] = self.error_class(["Неверный параметр для группы с регулярками"])

        if not rng_txt and self.group.default_type == "3":
            self._errors["rng_txt"] = self.error_class(["Обязательный параметр для группы с текстовыми блокировками"])

        if not rng_re and self.group.default_type == "4":
            self._errors["rng_re"] = self.error_class(["Обязательный параметр для группы с регулярками"])

        if rng_start and rng_end:
            self.cleaned_data["version"] = IPAddress(rng_start).version
        elif rng_txt:
            self.cleaned_data["version"] = "txt"
        elif rng_re:
            self.cleaned_data["version"] = "re"

        return super(EditRangeForm, self).clean()


class EditExceptionForm(forms.Form):

    def __init__(self, *args, **kwargs):
        super(EditExceptionForm, self).__init__(*args, **kwargs)
        group_id = self.data["group_id"]
        version = self.data["ip_version"]
        net_ip = self.data["net_ip"]
        net_mask = self.data["net_mask"]
        exceptions = get_exceptions_for_net(net_ip, net_mask, int(version), [group_id])
        choices = []
        for ex in exceptions:
            choices.append((str(ex.get_net()), str(ex.get_net())))
        self.fields["exceptions"] = forms.MultipleChoiceField(choices=choices, required=False, widget=forms.widgets.CheckboxSelectMultiple())

    add_exception = forms.CharField(required=False)
    del_exception = forms.CharField(required=False)

    ip_version = forms.IntegerField(initial=4, widget=forms.HiddenInput())
    group_id = forms.IntegerField(widget=forms.HiddenInput())

    net_ip = GenericIPAddressFieldRus(widget=forms.HiddenInput())
    net_mask = forms.IntegerField(widget=forms.HiddenInput())

    except_ip = GenericIPAddressFieldRus(required=False, widget=forms.TextInput(attrs={"placeholder": "Ip-адрес сети"}))
    except_mask = forms.IntegerField(required=False, widget=forms.TextInput(attrs={"class": "input-small", "placeholder": "Маска"}))
    exceptions = forms.ChoiceField(choices=())

    operation_descr = forms.CharField(required=False, widget=forms.Textarea(attrs={"class": "input-xlarge", "rows": 3, "placeholder": "Причина действия"}))

    def clean(self):
        del_exception = self.cleaned_data["del_exception"]
        add_exception = self.cleaned_data["add_exception"]
        if self._errors:
            return self.cleaned_data
        if del_exception and add_exception:
            msg = "Addition and deletion alltogether. Unbelievable."
            self._errors["del_excpetion"] = self.error_class([msg])
            self._errors["add_excpetion"] = self.error_class([msg])
            return self.cleaned_data
        if add_exception:
            except_ip = self.cleaned_data.get("except_ip")
            except_mask = self.cleaned_data.get("except_mask")
            if not except_ip:
                self._errors["except_ip"] = self.error_class(["Не указан ip-адрес исключения."])
            elif not except_mask:
                self._errors["except_mask"] = self.error_class(["Не указана маска для исключения."])
            else:
                except_version = IPAddress(except_ip).version
                if except_version != self.cleaned_data["ip_version"]:
                    self._errors["except_ip"] = self.error_class(["Неверная версия ip."])

        return self.cleaned_data


class IpInfoForm(forms.Form):
    ip = GenericIPAddressFieldRus()
