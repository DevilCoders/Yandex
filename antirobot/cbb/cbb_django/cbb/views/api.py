import functools
import ipaddress
import json
import logging
import time
from functools import wraps

import ticket_parser2.api.v1 as tp2

from django.conf import settings
from django import http
from django.core.cache import caches
from django.utils.translation import ugettext_lazy as _
from django.views.decorators.csrf import csrf_exempt

from antirobot.cbb.cbb_django.cbb.forms import api as forms
from antirobot.cbb.cbb_django.cbb.library import data, db
from antirobot.cbb.cbb_django.cbb.library.errors import ValidationError, NoGroupError
from antirobot.cbb.cbb_django.cbb.models import Group, BLOCK_VERSIONS


X_YA_SERVICE_TICKET_HEADER = "X-Ya-Service-Ticket"
logger = logging.getLogger(__name__)
tickets_logger = logging.getLogger("tvm_tickets")
HTTP_X_YA_SERVICE_TICKET = "HTTP_" + X_YA_SERVICE_TICKET_HEADER.replace("-", "_").upper()


class ClientPermissions:
    def __init__(self, client_config):
        assert client_config["groups"] == "all" or isinstance(client_config["groups"], list)
        self.client_id = client_config["client_id"]
        self.groups = client_config["groups"]
        self.name = client_config["name"]

    def is_allowed_group(self, group_id):
        if not settings.TVM_API_CHECK:
            return True
        return self.groups == "all" or int(group_id) in self.groups

    def get_display_name(self):
        return f"TVM:{self.client_id} ({self.name})"


class ClientAuthorizer:
    def __init__(self, file_name):
        self._clients = self._read_clients(file_name) if file_name else {}
        logger.info("Loaded %s client ids" % len(self._clients))
        self.tvm_client = None

    def get_tvm_client(self):
        if self.tvm_client is None:
            logger.debug("TVM client has been initiated (if you see it too often, it is bad for performance)")
            self.tvm_client = tp2.TvmClient(tp2.TvmApiClientSettings(
                self_client_id=settings.TVM_CLIENT_ID,
                enable_service_ticket_checking=True,
                enable_user_ticket_checking=tp2.BlackboxEnv.Prod,
            ))
        return self.tvm_client

    @staticmethod
    def _read_clients(file_name):
        with open(file_name) as inp:
            config = json.load(inp)

        return {client["client_id"]: ClientPermissions(client) for client in config["clients"]}

    def get_client_permissions(self, service_ticket):
        tvm_client = self.get_tvm_client()

        if service_ticket:
            client_id = int(tvm_client.check_service_ticket(service_ticket).src)
        else:
            client_id = -1

        return self._clients.get(client_id, None)


class NotAllowedError(Exception):
    def __init__(self, group_id, permissions):
        self.group_id = group_id
        self.permissions = permissions


client_authorizer = ClientAuthorizer(settings.TVM_ALLOWED_CLIENTS_FROM)


def form_cidr_result(ranges, with_descr):
    return "\n".join(filter(None, (rng.get_cidr_csv(with_descr=with_descr) for rng in ranges)))


class api_cache():
    """
    Decorator that caches responses by url and respects updates to relevant
    Groups - when that happens, cache is voided.
    """
    cache = caches["api-cache"]

    def __init__(self, view_func):
        self.func = view_func
        functools.update_wrapper(self, view_func)

    @db.main.use_slave()
    def __call__(self, request, *args, **kwargs):
        """
        Returns response from cache for request
        if cached request already is exist and not older than 15 minutes
        and its created time more than max updated time of flags.
        If not - creates new cached response for that request.
        """

        form = forms.BaseGetForm(request.GET)

        if not form.is_valid():
            return self.func(request, *args, **kwargs)

        group_ids = form.cleaned_data.get("flag")
        # Get the latest update time (float) for all groups in group_ids
        last_update = Group.latest_group_update_time(group_ids)

        # TODO: use something else as key? Hash from cleaned_data, maybe?
        key = request.get_full_path()

        if last_update and not settings.DEBUG:
            try:
                response_from_cache, created = self.cache.get(key)
            except (ValueError, TypeError):
                response_from_cache, created = None, None

            # last_updates resolution is a second, so we need to compensate it
            if created and (created - last_update > 1.):
                logging.debug("Answer for %r from cache" % group_ids)
                return response_from_cache

        response = self.func(request, *args, **kwargs)

        if response.status_code == 200:
            self.cache.set(key, (response, time.time()))

        return response


def api_view(func):
    @wraps(func)
    def _decorator(request, *args, **kwargs):
        try:
            service_ticket = request.META.get(HTTP_X_YA_SERVICE_TICKET)
            client_permissions = client_authorizer.get_client_permissions(service_ticket)

            tickets_logger.info("%(client_id)s\t%(ticket)s\t%(addr)s\t%(url)s" % {
                "client_id": client_permissions.client_id if client_permissions is not None else -1,
                "ticket": tp2.ServiceTicket.remove_signature(service_ticket) if service_ticket else "",
                "addr": request.META["REMOTE_ADDR"],
                "url": request.get_full_path()
            })

            if not settings.DEBUG and client_permissions is None:
                return http.HttpResponseForbidden("Invalid service ticket")

            return func(request, *args, **kwargs, permissions=client_permissions)
        except NotAllowedError as e:
            return http.HttpResponseForbidden(f"Client {e.permissions.client_id} have not appropriate permissions "
                                              f"for group {e.group_id}. "
                                              f"Allowed: {str(e.permissions.groups)}.")
        except Exception:
            if settings.DEBUG:
                raise

            logger.exception("Api view error at: %s" % request.get_full_path())

            return http.HttpResponse("Error\n")

    return _decorator


# ToDo: on rewriting api: return pairs: group, updated
@api_view
@db.main.use_slave()
def check_flag(request, permissions=None):
    form = forms.CheckFlagForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    groups_ids = form.cleaned_data["flag"]
    groups_updated = Group.get_groups_updated(groups_ids)
    if not groups_updated:
        raise NoGroupError(_("None of given groups found"))

    result = []
    for group in groups_ids:
        updated = groups_updated.get(group)
        if updated is None:
            result.append("")
        else:
            result.append(updated.strftime("%s"))

    content = ",".join(result)

    return http.HttpResponse(content)


@api_view
@api_cache
@db.main.use_slave()
def get_range(request, permissions=None):
    request.start_time = time.time()
    form = forms.GetRangeForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    rng_start = form.cleaned_data["range_src"]
    rng_end = form.cleaned_data["range_dst"]
    groups = form.cleaned_data["flag"]
    with_expire = form.cleaned_data["with_expire"]
    with_except = form.cleaned_data["with_except"]
    with_format = form.cleaned_data["with_format"]
    is_txt = ("range_txt" in with_format)
    is_re = ("range_re" in with_format)
    version_txt = form.cleaned_data["version"]
    versions = (4, 6) if version_txt == "all" else (int(version_txt),)

    if not is_txt and not is_re and ((rng_start and rng_end) or groups):
        ranges = []
        for ver in versions:
            ranges.extend(data.get_ranges(rng_start, rng_end, ver, groups=groups, exclude_expired=True))

        if with_format:
            result = "\n".join(rng.get_csv_with_format(with_format) for rng in ranges)
        else:
            result = "\n".join(rng.get_csv(with_expire, with_except) for rng in ranges)

    if groups and (is_txt or is_re):
        version = "txt" if is_txt else "re"
        rules = BLOCK_VERSIONS[version].get_by_groups(groups)

        if "rule_id" in with_format:
            rules = (f"rule_id={rule.id};{getattr(rule, rule.value_key)}" for rule in rules)
        else:
            rules = (f"{getattr(rule, rule.value_key)}" for rule in rules)

        result = "\n".join(rules)

    logging.debug("get_range tool %s seconds" % int(time.time() - request.start_time))
    return http.HttpResponse(result)


@api_view
@api_cache
@db.main.use_slave()
def get_netblock(request, permissions=None):
    form = forms.GetNetblockForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    groups = form.cleaned_data["flag"]
    version_txt = form.cleaned_data["version"]
    versions = (4, 6) if version_txt == "all" else (int(version_txt),)

    ranges = []
    for ver in versions:
        ranges.extend(data.get_ranges(groups=groups, version=ver, is_exception=False, exclude_expired=True))
    result = form_cidr_result(ranges, True)

    return http.HttpResponse(result)


@api_view
@api_cache
@db.main.use_slave()
def get_netexcept(request, permissions=None):
    form = forms.GetNetexceptForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    groups = form.cleaned_data["flag"]
    net_ip = form.cleaned_data["net_ip"]
    net_mask = form.cleaned_data["net_mask"]
    version_txt = form.cleaned_data["version"]
    versions = (4, 6) if version_txt == "all" else (int(version_txt),)

    ranges = []
    if net_ip and net_mask:
        for ver in versions:
            ranges.extend(data.get_exceptions_for_net(net_ip, net_mask, ver, groups))
    else:
        for ver in versions:
            ranges.extend(data.get_ranges(groups=groups, version=ver, is_exception=True, exclude_expired=True))

    result = form_cidr_result(ranges, False)
    return http.HttpResponse(result)


@api_view
@db.main.use_master(commit_on_exit=True)
def set_netblock(request, permissions=None):
    form = forms.SetNetblockForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    operation = form.cleaned_data["operation"]
    group_id = form.cleaned_data["flag"]
    net_ip = form.cleaned_data["net_ip"]
    net_mask = form.cleaned_data["net_mask"]
    description = form.cleaned_data["description"]
    version = int(form.cleaned_data["version"])
    user = permissions.get_display_name()

    if not permissions.is_allowed_group(group_id):
        raise NotAllowedError(group_id, permissions)

    group = Group.query.filter(Group.id == group_id, Group.is_active).one()
    if operation == "del":
        data.del_ip_range(net_ip, net_mask, group, user, description, version, net=True, scold=True)

    if operation == "add":
        data.add_net(net_ip, net_mask, group, description, user, version, False)
    return http.HttpResponse("Ok\n")


@api_view
@db.main.use_master(commit_on_exit=True)
def set_netexcept(request, permissions=None):
    form = forms.SetNetexceptForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    cleaned_data = form.cleaned_data

    operation = cleaned_data["operation"]
    group_id = cleaned_data["flag"]
    net_ip = cleaned_data["net_ip"]
    net_mask = cleaned_data["net_mask"]
    except_ip = cleaned_data["except_ip"]
    except_mask = cleaned_data["except_mask"]
    description = cleaned_data["description"]
    version = int(cleaned_data["version"])
    user = permissions.get_display_name()

    if not permissions.is_allowed_group(group_id):
        raise NotAllowedError(group_id, permissions)

    group = Group.query.filter(Group.id == group_id, Group.is_active).one()
    if operation == "add":
        data.add_del_netexcept(
            group=group, net_ip=net_ip, net_mask=net_mask,
            except_ip=except_ip, except_mask=except_mask,
            operation="add", block_descr=description,
            user=user, version=version
        )
    elif operation == "del":
        data.add_del_netexcept(
            group=group, net_ip=net_ip, net_mask=net_mask,
            except_ip=except_ip, except_mask=except_mask,
            operation="del", operation_descr=description,
            user=user, version=version
        )
    return http.HttpResponse("Ok\n")


@api_view
@db.main.use_master(commit_on_exit=True)
def set_range(request, permissions=None):
    form = forms.SetRangeForm(request.GET)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    rng_start = form.cleaned_data["range_src"]
    rng_end = form.cleaned_data["range_dst"]
    range_txt = form.cleaned_data["range_txt"]
    range_re = form.cleaned_data["range_re"]
    group_id = form.cleaned_data["flag"]
    operation = form.cleaned_data["operation"]
    description = form.cleaned_data["description"]
    expire = form.cleaned_data["expire"]
    version = int(form.cleaned_data["version"])
    user = permissions.get_display_name()

    if not permissions.is_allowed_group(group_id):
        raise NotAllowedError(group_id, permissions)

    group = Group.query.filter(Group.id == group_id, Group.is_active).one()
    if range_txt or range_re:
        if range_txt:
            block_version = "txt"
            what = range_txt
        else:
            block_version = "re"
            what = range_re

        if operation == "add":
            BLOCK_VERSIONS[block_version].add(what, group, block_descr=description, user=user, expire=expire, scold_on_repeat=False)
        elif operation == "del":
            BLOCK_VERSIONS[block_version].delete(group, what, user=user, operation_descr=description)
    elif rng_start and rng_end:
        if operation == "del":
            data.del_ip_range(rng_start, rng_end, group, user=user, net=False, operation_descr=description, scold=False)
        elif operation == "add":
            data.add_ip_range_common(
                rng_start, rng_end, group,
                block_descr=description, user=user,
                expire=expire, version=version,
                scold_on_repeat=False
            )
    return http.HttpResponse("Ok\n")


@api_view
@csrf_exempt
@db.main.use_master(commit_on_exit=True)
def set_ranges(request, permissions=None):
    form = forms.SetRangesForm(request.POST)

    if not form.is_valid():
        raise ValidationError(errors=form.errors)

    group_id = form.cleaned_data["flag"]
    rng_list = form.cleaned_data["rng_list"]
    description = form.cleaned_data["description"]

    if not permissions.is_allowed_group(group_id):
        raise NotAllowedError(group_id, permissions)

    user = permissions.get_display_name()
    group = Group.query.filter(Group.id == group_id, Group.is_active).one()
    for ip in rng_list:
        rng_start = ip
        rng_end = ip
        if isinstance(ipaddress.ip_address(ip), ipaddress.IPv4Address):
            version = 4
        elif isinstance(ipaddress.ip_address(ip), ipaddress.IPv6Address):
            version = 6

        data.add_ip_range_common(
            rng_start, rng_end, group,
            block_descr=description, user=user,
            expire=None, version=version,
            scold_on_repeat=False
        )

    return http.HttpResponse("Ok\n")
