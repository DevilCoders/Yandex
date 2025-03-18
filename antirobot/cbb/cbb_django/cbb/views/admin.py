import datetime
import json
import logging
import requests
import string
from functools import wraps

from antirobot.cbb.cbb_django.cbb.forms import admin as forms
from antirobot.cbb.cbb_django.cbb.library import data, db, errors, notify, antirobot_groups
from antirobot.cbb.cbb_django.cbb.library.common import (CIDR, RANGE, SINGLE_IP, TXT, RE, get_full_range, get_user, response_forbidden, get_bin_id)
from antirobot.cbb.cbb_django.cbb.models import (BLOCK_VERSIONS, HISTORY_VERSIONS, Group, GroupResponsible)

from django.conf import settings
from django.contrib import messages
from django.http import (HttpResponse, HttpResponseBadRequest, JsonResponse, HttpResponseRedirect)
from django.shortcuts import redirect, render
from django.utils.translation import ugettext as _

from sqlalchemy import desc
from sqlalchemy.orm.exc import NoResultFound

logger = logging.getLogger("cbb.views")


@db.main.use_slave()
def get_group_id(request, search_in_history=True):
    """
        Get group_id from request
    """
    rng_id = request.GET.get("rng_txt")
    version = "txt"
    if rng_id is None:
        rng_id = request.GET.get("rng_re")
        version = "re"

    if rng_id is not None:
        try:
            group_id = BLOCK_VERSIONS[version].find_one(rng_id).group_id
        except NoResultFound:
            if not search_in_history:
                return None
            group_id = HISTORY_VERSIONS[version].find_one(rule_id=rng_id).group_id
        return group_id
    return None


def user_can(action):
    def wrapped_user_can(foo):
        @wraps(foo)
        def wrapper(request, *args, **kwargs):
            group_id = kwargs.get("group_id")
            if (group_id is None):
                group_id = get_group_id(request)
            if request.cbb_user.can(action, group_id=group_id):
                return foo(request, *args, **kwargs)
            else:
                return response_forbidden()
        return wrapper
    return wrapped_user_can


@db.main.use_slave()
def should_redirect():
    def wrapped_should_rederect(foo):
        @wraps(foo)
        def wrapper(request, *args, **kwargs):
            group_id = kwargs.get("group_id")
            if group_id is not None:
                return foo(request, *args, **kwargs)
            group_id = get_group_id(request, search_in_history=False)
            if group_id is None:
                old_path = request.get_full_path()
                new_path = 'view_range'.join(old_path.split('edit_range'))
                return HttpResponseRedirect(new_path)
            return foo(request, *args, **kwargs)
        return wrapper
    return wrapped_should_rederect


def ranges_redirect(group_id, group_type, rng_start="", version=4):
    return redirect(f"/groups/{group_id}")


def set_group_editable(gr, request):
    gr.editable = False
    if request.cbb_user:
        gr.editable = request.cbb_user.can_modify_group(group_id=gr.id)
    return gr


@db.main.use_slave()
def groups(request, service=None, tag=None):
    antirobot_groups_info = antirobot_groups.AntirobotGroups()

    internal_groups = []
    external_groups = []
    old_groups = []

    for gr in Group.query.filter_by(is_active=True).order_by(Group.id).all():
        services = set(antirobot_groups_info.get_services(gr.id))
        tags_from_description = set()

        description = []
        for word in gr.group_descr.split():
            if word.startswith("#"):
                word_tag = word.strip(string.punctuation)
                tags_from_description.add(word_tag)
                description.append({"text": word, "kind": "tag", "tag": word_tag})
            elif antirobot_groups_info.is_known_service(word):
                services.add(word.lower())
                description.append({"text": word, "kind": "service", "service": word.lower()})
            else:
                description.append({"text": word, "kind": "none"})

        if (service and service in services) or (tag and tag in tags_from_description) or (not service and not tag) :
            set_group_editable(gr, request)
            gr.description = description

            if gr.is_internal:
                internal_groups.append(gr)
            elif gr.updated and gr.updated.year > datetime.datetime.now().year - 2:
                external_groups.append(gr)
            else:
                old_groups.append(gr)

    return render(
        request,
        "cbb/group/groups.html",
        {
            "cbb_user": request.cbb_user,
            "internal_groups": internal_groups,
            "old_groups": old_groups,
            "external_groups": external_groups,
        }
    )


def index(request):
    return redirect("/groups")


def staff(request, login):
    url = f"https://staff-api.yandex-team.ru/v3/persons?_pretty=0&_fields=login,name.first.ru,name.last.ru,images.photo&login={login}"
    response = requests.get(url, headers={"Authorization": f"OAuth {settings.STAFF_TOKEN}"})
    data = response.json()
    return JsonResponse(data)


class RangesPage(object):

    per_page = 40

    def __init__(self, queryset, page_num):
        self.page_num = page_num
        self.entries = list(
            queryset[self.per_page * (self.page_num - 1):self.per_page * self.page_num + 1])

        # 1 more is left to determine if there is a new ^^^^ page
        if len(self.entries) > self.per_page:
            self.next = self.page_num +1
            self.entries = self.entries[:self.per_page]
        else:
            self.next = None

    @property
    def previous(self):
        if self.page_num <= 1:
            return None
        return self.page_num - 1


@user_can(settings.ACTION_GROUP_MODIFY)
@db.main.use_slave()
def view_group_blocks(request, group_id):
    page = request.GET.get("page")
    version = None
    # ToDo: add pages
    group = Group.query.get(group_id)

    if group.default_type in ("3", "4"):
        version = "txt" if group.default_type == "3" else "re"
        output = BLOCK_VERSIONS[version].q_find(groups=[group_id]).order_by(desc(BLOCK_VERSIONS[version].created))
    else:
        version = 6 if "ipv6" in request.GET else 4
        output = data.get_ranges(version=version, groups=[group_id], get_all=False, is_exception=False, exclude_expired=True) \
            .order_by(desc(BLOCK_VERSIONS[version].created))

    try:
        page = int(page)
        assert page > 0
    except (ValueError, TypeError, AssertionError):
        page = 1
    output_pages = RangesPage(output, page)

    set_group_editable(group, request)
    return render(
        request,
        "cbb/group/blocks_list.html",
        {
            "cbb_user": request.cbb_user,
            "output_pages": output_pages,
            "version": version,
            "group": group
        }
    )


@user_can(settings.ACTION_GROUP_MODIFY)
@db.main.use_master(commit_on_exit=True)
@notify.notify_responsibles
def delete_group(request, group_id=None):
    if request.method == "POST":
        try:
            group = Group.query.filter_by(id=group_id).one()
        except errors.NoResultFound:
            raise errors.NoGroupError(_("No such group found."))

        if group.has_undeleted_ranges():
            messages.error(request, _("Chosen group has undeleted ranges"))
        elif group.has_ip_from_links():
            messages.error(request, _(f"There is backlinks to this group with 'ipfrom={group_id}', <a href=\"/search?search=ip_from%3D{group_id}\">search</a> and delete them first"))
        else:
            group.is_active=False
            for row in GroupResponsible.query.filter_by(group_id=group.id).all():
                db.main.session.delete(row)

            db.main.session.flush()
            request.cbb_changes.log_group_removed(group.id)
            messages.success(request, _("Group deleted successfully"))
        return redirect("/groups")


@notify.notify_responsibles
def do_edit_group(request, group_id=None, edit_mode=True):
    if request.method == "POST":
        form = forms.ChangeGroupInfoForm(request.POST)
        if form.is_valid():
            group_id = form.cleaned_data["group_id"]
            is_internal = form.cleaned_data["is_internal"]
            name = form.cleaned_data["name"]
            group_descr = form.cleaned_data["group_descr"]
            default_type = form.cleaned_data["default_type"]
            if default_type in (SINGLE_IP, TXT):
                period_to_expire = form.cleaned_data["period_to_expire"]
            else:
                period_to_expire = None

            responsible_people = json.loads(form.cleaned_data["responsible_people"])
            if group_id is None:
                group = Group(
                    name=name,
                    group_descr=group_descr,
                    is_internal=is_internal,
                    updated=datetime.datetime.now(),
                    default_type=default_type,
                    period_to_expire=period_to_expire
                )
            else:
                group = Group.query.filter_by(id=group_id).one()
                group.name = name
                group.updated = datetime.datetime.now()
                group.group_descr = group_descr
                group.period_to_expire = period_to_expire

            db.main.session.add(group)
            db.main.session.flush()
            if group.id is None:
                db.main.session.refresh(group)

            if group_id is None:
                request.cbb_changes.log_group_added(group.id, group.name, group.period_to_expire, group.group_descr)
            else:
                request.cbb_changes.log_metadata_change(group.id, group.name, group.period_to_expire, group.group_descr)

            for login in responsible_people["added"]:
                db.main.session.add(GroupResponsible(group_id=group.id, user_login=login))
                request.cbb_changes.log_responsible_added(group.id, login)

            for login in responsible_people["deleted"]:
                row = GroupResponsible.query.filter_by(group_id=group.id, user_login=login).one()
                if row:
                    db.main.session.delete(row)
                    request.cbb_changes.log_responsible_removed(group.id, login)

            messages.success(request, _("Group saved successfully"))

        return redirect("/groups")

    responsible_people = {
        "unchanged": [],
        "added": [],
        "deleted": [],
    }

    if group_id is None:
        responsible_people["added"] = [request.cbb_user.login]
        form = forms.ChangeGroupInfoForm(initial={"responsible_people": json.dumps(responsible_people)})
        edit_mode = True
    else:
        group = Group.query.filter_by(id=group_id).one()
        responsible_people["unchanged"] = GroupResponsible.get_responsibles(group_id)

        initial = {
            "name": group.name,
            "group_descr": group.group_descr,
            "is_internal": group.is_internal,
            "default_type": group.default_type,
            "group_id": group.id,
            "period_to_expire": group.get_period_to_expire(),
            "responsible_people": json.dumps(responsible_people),
        }
        form = forms.ChangeGroupInfoForm(initial=initial)
        if not edit_mode:
            form.make_readonly()

    return render(
        request,
        "cbb/group/edit_group.html",
        {
            "cbb_user": request.cbb_user,
            "form": form,
            "edit_mode": edit_mode
        },
    )


@user_can(settings.ACTION_GROUP_MODIFY)
@db.main.use_master(commit_on_exit=True)
def edit_group(request, group_id=None):
    return do_edit_group(request, group_id)


@user_can(settings.ACTION_GROUP_CREATE)
@db.main.use_master(commit_on_exit=True)
def add_group(request):
    return do_edit_group(request, None)


@user_can(settings.ACTION_GROUP_CREATE)
@db.main.use_master(commit_on_exit=True)
@notify.notify_responsibles
def add_service(request):
    if request.method == 'POST':
        form = forms.AddServiceForm(request.POST)
        if form.is_valid():
            responsible_people = json.loads(form.cleaned_data["responsible_people"])
            service = form.cleaned_data["name"]
            groups_to_create = [
                [f'{service}_captcha_ip', f'Форсированная капча с протуханием для {service} по IP #cbb{service}', "1"],
                [f'{service}_captcha_re', f'Форсированная капча с протуханием для {service} по RE #cbb{service}', "3"],
                [f'{service}_403_ip', f'Форсированный 403 с протуханием для {service} по IP #cbb{service}', "1"],
                [f'{service}_403_re', f'Форсированный 403 с протуханием для {service} по RE #cbb{service}', "3"],
                [f'{service}_mark_re', f'Разметка по RE для {service} #cbb{service}', "3"],
            ]
            groups = []
            for name, descr, default_type in groups_to_create:
                group = Group(
                    name=name,
                    group_descr=descr,
                    is_internal=None,
                    updated=datetime.datetime.now(),
                    default_type=default_type,
                    period_to_expire=None,
                )
                db.main.session.add(group)
                groups.append(group)

            db.main.session.flush()
            for group in groups:
                db.main.session.refresh(group)

            for login in responsible_people["added"]:
                for group in groups:
                    db.main.session.add(GroupResponsible(group_id=group.id, user_login=login))
                    request.cbb_changes.log_responsible_added(group.id, login)

            messages.success(request, _(f"Groups for {service} service successfully"))
            return redirect("/groups")

    responsible_people = {
        "unchanged": [],
        "added": [request.cbb_user.login],
        "deleted": [],
    }
    form = forms.ChangeGroupInfoForm(initial={"responsible_people": json.dumps(responsible_people)})
    return render(
        request,
        "cbb/group/add_service.html",
        {
            "cbb_user": request.cbb_user,
            "form": form,
        },
    )


@user_can(settings.ACTION_READ)
@db.main.use_slave()
def view_group(request, group_id):
    return do_edit_group(request, group_id, edit_mode=False)


@should_redirect()
@user_can(settings.ACTION_GROUP_MODIFY)
@db.main.use_master(commit_on_exit=True)
@notify.notify_responsibles
def edit_range(request, group_id=None):
    if group_id is None:
        group_id = get_group_id(request)
        if group_id is None:
            return HttpResponseBadRequest()

    user = get_user(request)
    group = Group.query.filter_by(id=group_id).one()
    if request.method == "GET":
        form = forms.EditRangeForm(request.GET, group=group)
        if form.is_valid():
            rng_txt_id = form.cleaned_data.get("rng_txt")
            rng_re_id = form.cleaned_data.get("rng_re")
            rng_start = form.cleaned_data.get("rng_start")
            rng_end = form.cleaned_data.get("rng_end")
            version = form.cleaned_data.get("version")
            if version == "txt":
                try:
                    block = BLOCK_VERSIONS[version].find_one(rng_txt_id)
                except NoResultFound:
                    return redirect(f"/groups/{group_id}/view_range?rng_txt={rng_txt_id}")
            elif version == "re":
                try:
                    block = BLOCK_VERSIONS[version].find_one(rng_re_id)
                except NoResultFound:
                    return redirect(f"/groups/{group_id}/view_range?rng_re={rng_re_id}")
            else:
                block = BLOCK_VERSIONS[version].find_one(group, rng_start, rng_end)

            return render(request, "cbb/range/edit_range.html", {
                "cbb_user": request.cbb_user,
                "group": group,
                "rng": block,
                "form": form,
                "is_god": request.cbb_user._is_god,
            })

        return HttpResponseBadRequest("Bad request: %s" % form.errors)

    if request.method == "POST":
        form = forms.EditRangeForm(request.POST, group=group)

        del_blocking = request.POST.get("del_bl")
        if del_blocking is not None and form.is_valid():
            description = form.cleaned_data.get("operation_descr")
            version = form.cleaned_data["version"]

            # TODO: try-except here
            if group.default_type in (TXT, RE):
                rng_txt_id = form.cleaned_data[f"rng_{version}"]
                BLOCK_VERSIONS[version].delete(group, txt_id=rng_txt_id, user=user, operation_descr=description)
                request.cbb_changes.log_txt_range_removed(group.id, rng_txt_id, description)
            else:
                rng_start = form.cleaned_data["rng_start"]
                rng_end = form.cleaned_data["rng_end"]
                data.del_ip_range(rng_start, rng_end, group, user=user, operation_descr=description, version=version, net=False)
                request.cbb_changes.log_ip_range_removed(group.id, rng_start, rng_end, description)

        move_blocking = request.POST.get("mov_bl")
        copy_blocking = request.POST.get("copy_bl")
        if (move_blocking is not None or copy_blocking is not None) and form.is_valid():
            description = form.cleaned_data.get("move_descr")
            new_expire = form.cleaned_data["new_expire"]
            new_group_id = form.cleaned_data["new_group"]
            exp_bin = form.cleaned_data["exp_bin"]
            if exp_bin is None or len(exp_bin) == 0:
                exp_bin = form.cleaned_data["exp_bin_for_services"]

            if new_group_id is None:
                return HttpResponseBadRequest()
            new_group = Group.query.filter_by(id=new_group_id).one()

            if group.default_type in (TXT, RE):
                version = "txt" if group.default_type == TXT else "re"
                rng_id = form.cleaned_data[f"rng_{version}"]

                block = BLOCK_VERSIONS[version].find_one(rng_id)
                old_bin = get_bin_id(block)

                if move_blocking is not None:
                    BLOCK_VERSIONS[version].move(user, group, new_group, block, description, new_expire, old_bin, exp_bin)
                    request.cbb_changes.log_txt_range_moved(group.id, new_group_id, rng_id, description, old_bin, exp_bin)

                elif copy_blocking is not None:
                    BLOCK_VERSIONS[version].copy(user, group, new_group, block, description, new_expire, old_bin, exp_bin)
                    request.cbb_changes.log_txt_range_copied(group.id, new_group_id, rng_id, description, old_bin, exp_bin)

    return redirect(f"/groups/{group_id}")


@db.main.use_slave()
@db.history.use_slave()
def view_range(request, group_id=None):
    if group_id is None:
        group_id = get_group_id(request)
        if group_id is None:
            return HttpResponseBadRequest()

    group = Group.query.filter_by(id=group_id).one()
    form = forms.EditRangeForm(request.GET, group=group)
    if form.is_valid():
        rng_txt_id = form.cleaned_data.get("rng_txt")
        rng_re_id = form.cleaned_data.get("rng_re")
        rng_start = form.cleaned_data.get("rng_start")
        rng_end = form.cleaned_data.get("rng_end")
        version = form.cleaned_data.get("version")

        block = None
        removed_block = None
        history_blocks = None

        if version == "txt" or version == "re":
            if version == "txt":
                rng_id = rng_txt_id
            else:
                rng_id = rng_re_id

            history_blocks = HISTORY_VERSIONS[version].get_txt_block_history(rule_id=rng_id)
            try:
                block = BLOCK_VERSIONS[version].find_one(rng_id)
            except NoResultFound:
                removed_block = HISTORY_VERSIONS[version].find_one(rule_id=rng_id)
        else:
            block = BLOCK_VERSIONS[version].find_one(group, rng_start, rng_end)

        if block is not None:
            return render(
                request, "cbb/range/view_range.html",
                {
                    "cbb_user": request.cbb_user,
                    "rng": block,
                    "group": group,
                    "history": history_blocks,
                    "editable": request.cbb_user.can_modify_group(group_id=group.id)
                }
            )
        else:
            if history_blocks is None or len(history_blocks) == 0:
                history_blocks = [removed_block]

            return render(
                request, "cbb/blocks_list/history.html",
                {
                    "cbb_user": request.cbb_user,
                    "version": version,
                    "version_to_output": {version: [{
                        "group": group,
                        "block": getattr(removed_block, removed_block.value_key),
                        "history": history_blocks,
                    }]},
                }
            )

    return HttpResponseBadRequest()


@user_can(settings.ACTION_GROUP_MODIFY)
@db.main.use_master(commit_on_exit=True)
@notify.notify_responsibles
def add_range(request, group_id=None):
    group = Group.query.filter_by(id=group_id).one()
    rng_type = group.default_type
    error = None
    error_list = None
    form_types = {
        RANGE: forms.AddRangeIpForm,
        SINGLE_IP: forms.AddSingleIpForm,
        CIDR: forms.AddNetForm,
        TXT: forms.AddBlockTxtForm,
        RE: forms.AddBlockReForm,
    }
    if request.method == "POST":
        form = form_types[rng_type](request.POST)
        if not form.is_valid():
            return render(request, "cbb/range/add_range.html", {"cbb_user": request.cbb_user, "form": form, "group": group})

        block_descr = form.cleaned_data["block_descr"]
        version = form.cleaned_data["ip_version"]
        user = get_user(request)

        rng_list = form.cleaned_data.get("rng_list")

        if rng_type in (RANGE, SINGLE_IP):
            rng_start = form.cleaned_data["rng_start"]
            rng_end = form.cleaned_data["rng_end"]
            expire = form.cleaned_data.get("expire")
            if rng_list:
                error_list = []
                new_rng_list = []
                for rng_start, rng_end, version in rng_list:
                    try:
                        data.add_ip_range_common(rng_start, rng_end, group, block_descr, user, expire, version)
                        request.cbb_changes.log_ip_range_added(group.id, rng_start, rng_end, expire, block_descr)
                    except Exception as e:
                        if isinstance(e, errors.CbbError):
                            msg = str(e)
                        else:
                            msg = _("Range format error")
                        error_list.append(msg + ": " + get_full_range(rng_start, rng_end))
                        new_rng_list.append(get_full_range(rng_start, rng_end))

                if error_list:
                    error = _("Some ranges were not added!")
                    form = form_types[rng_type]({
                        "block_descr": block_descr,
                        "ip_version": version,
                        "expire": expire,
                        "rng_list": "\r\n".join(new_rng_list)
                    })
                else:
                    return ranges_redirect(group.id, group.kind())
            else:
                try:
                    result = data.add_ip_range_common(rng_start, rng_end, group, block_descr, user, expire, version)
                    if result:
                        return ranges_redirect("all", "ip", rng_start, version)
                    else:
                        request.cbb_changes.log_ip_range_added(group.id, rng_start, rng_end, expire, block_descr)
                        return ranges_redirect(group.id, group.kind())
                except Exception as e:
                    if isinstance(e, errors.CbbError):
                        error = str(e)
                    else:
                        error = _("Blocking was not added")
                    logger.error(e)
        if rng_type == CIDR:
            net_ip = form.cleaned_data["net_ip"]
            net_mask = form.cleaned_data["net_mask"]
            rng_list = form.cleaned_data["rng_list"]
            if rng_list:
                error_list = []
                new_rng_list = []
                for net_ip, net_mask, version in rng_list:
                    try:
                        data.add_net(net_ip, net_mask, group, block_descr, user, version)
                        request.cbb_changes.log_network_added(group.id, net_ip, net_mask, block_descr)
                    except Exception as e:
                        if isinstance(e, errors.CbbError):
                            msg = str(e)
                        else:
                            msg = _("Range format error")
                        error_list.append(msg + ": " + net_ip + "/" + net_mask)
                        new_rng_list.append(net_ip + "/" + net_mask)
                if error_list:
                    error = _("Some ranges were not added!")
                    form = form_types[rng_type]({
                        "block_descr": block_descr,
                        "ip_version": version,
                        "rng_list": "\r\n".join(new_rng_list)
                    })
                else:
                    return ranges_redirect(group.id, group.kind())
            else:
                try:
                    result = data.add_net(net_ip, net_mask, group, block_descr, user, version)
                    if result:
                        net = data.get_range_from_net(net_ip, net_mask, version, "ip")
                        return ranges_redirect("all", "ip", str(net.ip), version)
                    else:
                        request.cbb_changes.log_network_added(group.id, net_ip, net_mask, block_descr)
                        return ranges_redirect(group.id, group.kind())
                except Exception as e:
                    if isinstance(e, errors.CbbError):
                        error = str(e)
                    else:
                        error = _("Blocking was not added")
                    logger.error(e)
        if rng_type in (TXT, RE):
            rng_type_str = "txt" if rng_type == TXT else "re"
            rng_txt = form.cleaned_data[f"rng_{rng_type_str}"]
            expire = form.cleaned_data.get("expire")

            if rng_type == RE:
                rng_txt = [line for line in (line.strip() for line in rng_txt.split("\n")) if line]

            try:
                BLOCK_VERSIONS[rng_type_str].add(rng_txt, group, block_descr, user, expire=expire)
                request.cbb_changes.log_txt_range_added(group.id, rng_txt, expire, block_descr)
                return ranges_redirect(group.id, group.kind())
            except Exception as e:
                if isinstance(e, errors.CbbError):
                    error = str(e)
                else:
                    error = _("Blocking was not added")
                logger.error(e)

    else:
        form = form_types[rng_type]()

    return render(request, "cbb/range/add_range.html", {"cbb_user": request.cbb_user, "form": form, "group": group, "error": error, "error_list": error_list})


@db.main.use_slave()
@db.history.use_slave()
def search(request):
    """
    Search combinded history and active blocks result.
    """
    if request.method == "GET":
        form = forms.FindBlockForm(request.GET)
        if form.is_valid():
            version = form.cleaned_data["version"]
            search = form.cleaned_data["search"]
            version_to_output = {}

            if version in (4, 6):
                version_to_output[str(version)] = HISTORY_VERSIONS[version].prepare_output(search)

            for other_version in ("txt", "re"):
                version_to_output[other_version] = HISTORY_VERSIONS[other_version].prepare_output(search)

            for output in version_to_output.values():
                for item in output:
                    set_group_editable(item["group"], request)

            return render(
                request,
                "cbb/blocks_list/history.html",
                {
                    "cbb_user": request.cbb_user,
                    "version": version,
                    "version_to_output": version_to_output,
                    "search": search
                }
            )
    return render(
        request,
        "cbb/blocks_list/history.html",
        {
            "cbb_user": request.cbb_user,
        }
    )


def status(request):
    """
    Return json containing current database statuses.
    """
    refresh = True if request.GET.get("refresh") else False
    res = {
        "refresh": refresh,
        "main": db.main.db_statuses(refresh=refresh),
        "history": db.history.db_statuses(refresh=refresh),
    }
    return HttpResponse(json.dumps(res, indent=4), content_type="application/json")
