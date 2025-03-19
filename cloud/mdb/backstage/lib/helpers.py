import json
import base64
import logging

import django.template
import django.core.paginator as dcp
import django.utils.safestring as dus


logger = logging.getLogger('backstage.lib.helpers')


class LinkedMixin:
    @classmethod
    def _class_app(self):
        return self._meta.app_label

    @classmethod
    def _class_verbose_name(self):
        return self._meta.verbose_name

    @classmethod
    def _class_verbose_name_plural(self):
        return self._meta.verbose_name_plural

    @classmethod
    def _class_model(self):
        return self._meta.verbose_name_plural.lower().replace(' ', '_')

    @classmethod
    def _class_objects_url(self):
        return f'/ui/{self._class_app()}/{self._class_model()}'

    @classmethod
    def _class_js_array(self):
        return f'{self._class_app()}_{self._class_model()}_selected'

    @property
    def self_app(self):
        return self._class_app()

    @property
    def self_app_model(self):
        return f'{self._class_app()}_{self._class_model()}'

    @property
    def self_primary_key(self):
        return self.pk

    @property
    def self_model(self):
        return self._class_model()

    @property
    def self_js_array(self):
        return self._class_js_array()

    @property
    def self_objects_url(self):
        return self._class_objects_url()

    @property
    def self_objects_ajax_url(self):
        return f'/ui/{self.self_app}/ajax/{self.self_model}'

    @property
    def self_url(self):
        return f'{self.self_objects_url}/{self.self_primary_key}'

    @property
    def self_ajax_url(self):
        return f'{self.self_objects_ajax_url}/{self.self_primary_key}'

    @property
    def self_link(self):
        return dus.mark_safe(
            f'<a href="{self.self_url}">{self}</a>'
        )

    @property
    def self_link_c(self):
        copy_html = get_simple_copy_html(self)
        return dus.mark_safe(
            f'<a href="{self.self_url}">{self}</a>&nbsp;{copy_html}'
        )

    @property
    def self_pk_c(self):
        copy_html = get_simple_copy_html(self)
        return dus.mark_safe(
            f'{self}&nbsp;{copy_html}'
        )

    @property
    def self_pk_link(self):
        return dus.mark_safe(
            f'<a href="{self.self_url}">{self.self_primary_key}</a>'
        )

    @property
    def self_pk_link_c(self):
        copy_html = get_simple_copy_html(self)
        return dus.mark_safe(
            f'<a href="{self.self_url}">{self.self_primary_key}</a>&nbsp;{copy_html}'
        )

    @property
    def self_ext_link(self):
        meta = self.__class__._meta
        return dus.mark_safe(
            f'<a href="{self.self_url}">{meta.app_label.title()} -> {meta.verbose_name_plural.title()} -> {self}</a>'
        )


class ActionResult:
    def __init__(
        self,
        obj=None,
        message=None,
        error=None,
    ):
        if not error and not message:
            raise ValueError("Invalid action result: either message or error should be provided")

        if error:
            self.result = False
        else:
            self.result = True
        self.obj = obj
        self.error = error
        self.message = message

    def __bool__(self):
        return self.result

    @property
    def obj_error_html(self):
        if not self.obj:
            return self.error
        else:
            return f'Error for <a href="{self.obj.self_url}">{self.obj}</a>: {self.error}'

    @property
    def obj_msg_html(self):
        if not self.obj:
            return self.message
        else:
            return f'Subackstageess for <a href="{self.obj.self_url}">{self.obj}</a>: {self.message}'


def render_html(template_path, context, request):
    template = django.template.loader.get_template(template_path)
    return template.render(context, request)


def paginator_setup(queryset, request, items_per_page=25):
    try:
        pagenum = int(request.GET.get('page', 1))
    except ValueError:
        pagenum = 1

    paginator = dcp.Paginator(queryset, items_per_page)

    try:
        objects = paginator.page(pagenum)
    except (dcp.EmptyPage, dcp.InvalidPage):
        objects = paginator.page(paginator.num_pages)

    page_range = paginator.page_range
    num_pages = paginator.num_pages
    if pagenum > num_pages:
        pagenum = num_pages

    return objects, page_range, num_pages, pagenum, paginator


def paginator_setup_set_context(
    queryset,
    request,
    context,
    objects_key='objects',
    items_per_page=25,
):
    objects, page_range, num_pages, pagenum, paginator = paginator_setup(
        queryset,
        request,
        items_per_page=items_per_page,
    )
    keys = [objects_key, 'page_range', 'num_pages', 'pagenum', 'paginator']
    for key in keys:
        if key in context:
            raise ValueError(f'Error: context already contains key {key}')

    context[objects_key] = objects
    context['page_range'] = page_range
    context['num_pages'] = num_pages
    context['pagenum'] = pagenum
    context['paginator'] = paginator

    return context


def get_simple_copy_html(value):
    value = str(value)
    value = base64.b64encode(value.encode()).decode()
    return f'<i title="Copy to clipboard" class="far fa-copy fa-fw noodle-js-copy" data-noodle-toggle="copy" data-value="{value}"></i>'


def get_post_data(request):
    post_data = json.loads(request.POST.get('data'))
    return post_data.get('params')


def get_client_ip(request):
    ip = ''
    try:
        x_forwarded_for = request.META.get('HTTP_X_FORWARDED_FOR', '')
        if x_forwarded_for:
            ip = x_forwarded_for.split(',')[0]
        else:
            ip = request.META.get('REMOTE_ADDR', '')
    except Exception as err:
        message = "Failed to get client ip address: {}".format(err)
        logger.exception(message)
    return ip


def get_request_id(request):
    return request.META.get('HTTP_X_REQ_ID', 'request-id-was-not-provided')
