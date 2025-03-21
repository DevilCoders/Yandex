"""
Module where admin tools dashboard modules classes are defined.
"""

from django.apps import apps as django_apps
from django.forms.utils import flatatt
from django.utils.itercompat import is_iterable  # noqa
from django.utils.text import capfirst  # noqa

from admin_tools.utils import AppListElementMixin, uniquify

try:
    # we use django.urls import as version detection as it will fail on django 1.11 and thus we are safe to use
    # gettext_lazy instead of ugettext_lazy instead
    from django.urls import reverse
    from django.utils.translation import gettext_lazy as _
except ImportError:
    from django.core.urlresolvers import reverse  # noqa
    from django.utils.translation import ugettext_lazy as _  # noqa


class DashboardModule(object):
    """
    Base class for all dashboard modules.
    Dashboard modules have the following properties:

    ``enabled``
        Boolean that determines whether the module should be enabled in
        the dashboard by default or not. Default value: ``True``.

    ``draggable``
        Boolean that determines whether the module can be draggable or not.
        Draggable modules can be re-arranged by users. Default value: ``True``.

    ``collapsible``
        Boolean that determines whether the module is collapsible, this
        allows users to show/hide module content. Default: ``True``.

    ``deletable``
        Boolean that determines whether the module can be removed from the
        dashboard by users or not. Default: ``True``.

    ``title``
        String that contains the module title, make sure you use the django
        gettext functions if your application is multilingual.
        Default value: ''.

    ``title_url``
        String that contains the module title URL. If given the module
        title will be a link to this URL. Default value: ``None``.

    ``css_classes``
        A list of css classes to be added to the module ``div`` class
        attribute. Default value: ``None``.

    ``pre_content``
        Text or HTML content to display above the module content.
        Default value: ``None``.

    ``content``
        The module text or HTML content. Default value: ``None``.

    ``post_content``
        Text or HTML content to display under the module content.
        Default value: ``None``.

    ``template``
        The template to use to render the module.
        Default value: 'admin_tools/dashboard/module.html'.
    """

    template = 'admin_tools/dashboard/module.html'
    enabled = True
    draggable = True
    collapsible = True
    deletable = True
    show_title = True
    title = ''
    title_url = None
    css_classes = None
    pre_content = None
    post_content = None
    children = None
    id = None

    def __init__(self, title=None, **kwargs):
        if title is not None:
            self.title = title

        for key in kwargs:
            if hasattr(self.__class__, key):
                setattr(self, key, kwargs[key])

        self.children = self.children or []
        self.css_classes = self.css_classes or []
        # boolean flag to ensure that the module is initialized only once
        self._initialized = False

    def init_with_context(self, context):
        """
        Like for the :class:`~admin_tools.dashboard.Dashboard` class, dashboard
        modules have a ``init_with_context`` method that is called with a
        ``django.template.RequestContext`` instance as unique argument.

        This gives you enough flexibility to build complex modules, for
        example, let's build a "history" dashboard module, that will list the
        last ten visited pages::

            from admin_tools.dashboard import modules

            class HistoryDashboardModule(modules.LinkList):
                title = 'History'

                def init_with_context(self, context):
                    request = context['request']
                    # we use sessions to store the visited pages stack
                    history = request.session.get('history', [])
                    for item in history:
                        self.children.append(item)
                    # add the current page to the history
                    history.insert(0, {
                        'title': context['title'],
                        'url': request.META['PATH_INFO']
                    })
                    if len(history) > 10:
                        history = history[:10]
                    request.session['history'] = history

        Here's a screenshot of our history item:

        .. image:: images/history_dashboard_module.png
        """
        pass

    def is_empty(self):
        """
        Return True if the module has no content and False otherwise.

        >>> mod = DashboardModule()
        >>> mod.is_empty()
        True
        >>> mod.pre_content = 'foo'
        >>> mod.is_empty()
        False
        >>> mod.pre_content = None
        >>> mod.is_empty()
        True
        >>> mod.children.append('foo')
        >>> mod.is_empty()
        False
        >>> mod.children = []
        >>> mod.is_empty()
        True
        """
        return self.pre_content is None and self.post_content is None and len(self.children) == 0

    def render_css_classes(self):
        """
        Return a string containing the css classes for the module.

        >>> mod = DashboardModule(enabled=False, draggable=True,
        ...                       collapsible=True, deletable=True)
        >>> mod.render_css_classes()
        'dashboard-module disabled draggable collapsible deletable'
        >>> mod.css_classes.append('foo')
        >>> mod.render_css_classes()
        'dashboard-module disabled draggable collapsible deletable foo'
        >>> mod.enabled = True
        >>> mod.render_css_classes()
        'dashboard-module draggable collapsible deletable foo'
        """
        ret = ['dashboard-module']
        if not self.enabled:
            ret.append('disabled')
        if self.draggable:
            ret.append('draggable')
        if self.collapsible:
            ret.append('collapsible')
        if self.deletable:
            ret.append('deletable')
        ret += self.css_classes
        return ' '.join(ret)

    def _prepare_children(self):
        pass


class Group(DashboardModule):
    """
    Represents a group of modules, the group can be displayed in tabs,
    accordion, or just stacked (default).
    As well as the :class:`~admin_tools.dashboard.modules.DashboardModule`
    properties, the :class:`~admin_tools.dashboard.modules.Group`
    has two extra properties:

    ``display``
        A string determining how the group should be rendered, this can be one
        of the following values: 'tabs' (default), 'accordion' or 'stacked'.

    ``force_show_title``
        Default behaviour for Group module is to force children to always show
        the title if Group has ``display`` = ``stacked``. If this flag is set
        to ``False``, children title is shown according to their``show_title``
        property. Note that in this case is children responsibility to have
        meaningful content if no title is shown.

    Here's an example of modules group::

        from admin_tools.dashboard import modules, Dashboard

        class MyDashboard(Dashboard):
            def __init__(self, **kwargs):
                Dashboard.__init__(self, **kwargs)
                self.children.append(modules.Group(
                    title="My group",
                    display="tabs",
                    children=[
                        modules.AppList(
                            title='Administration',
                            models=('django.contrib.*',)
                        ),
                        modules.AppList(
                            title='Applications',
                            exclude=('django.contrib.*',)
                        )
                    ]
                ))

    The screenshot of what this code produces:

    .. image:: images/dashboard_module_group.png
    """

    force_show_title = True
    template = 'admin_tools/dashboard/modules/group.html'
    display = 'tabs'

    def init_with_context(self, context):
        if self._initialized:
            return
        for module in self.children:
            # to simplify the whole stuff, modules have some limitations,
            # they cannot be dragged, collapsed or closed
            module.collapsible = False
            module.draggable = False
            module.deletable = False
            if self.force_show_title:
                module.show_title = self.display == 'stacked'
            module.init_with_context(context)
        self._initialized = True

    def is_empty(self):
        """
        A group of modules is considered empty if it has no children or if
        all its children are empty.

        >>> from admin_tools.dashboard.modules import DashboardModule, LinkList
        >>> mod = Group()
        >>> mod.is_empty()
        True
        >>> mod.children.append(DashboardModule())
        >>> mod.is_empty()
        True
        >>> mod.children.append(LinkList('links', children=[
        ...    {'title': 'example1', 'url': 'http://example.com'},
        ...    {'title': 'example2', 'url': 'http://example.com'},
        ... ]))
        >>> mod.is_empty()
        False
        """
        if super(Group, self).is_empty():
            return True
        for child in self.children:
            if not child.is_empty():
                return False
        return True

    def _prepare_children(self):
        # computes ids for children: generates them if they are not set
        # and then prepends them with this group's id
        seen = set()
        for id, module in enumerate(self.children):
            proposed_id = "%s_%s" % (self.id, module.id or id + 1)
            module.id = uniquify(proposed_id, seen)
            module._prepare_children()


class LinkList(DashboardModule):
    """
    A module that displays a list of links.
    As well as the :class:`~admin_tools.dashboard.modules.DashboardModule`
    properties, the :class:`~admin_tools.dashboard.modules.LinkList` takes
    an extra keyword argument:

    ``layout``
        The layout of the list, possible values are ``stacked`` and ``inline``.
        The default value is ``stacked``.

    Link list modules children are simple python dictionaries that can have the
    following keys:

    ``title``
        The link title.

    ``url``
        The link URL.

    ``external``
        Boolean that indicates whether the link is an external one or not.

    ``description``
        A string describing the link, it will be the ``title`` attribute of
        the html ``a`` tag.

    ``attrs``
        Hash comprising attributes of the html ``a`` tag.

    Children can also be iterables (lists or tuples) of length 2, 3, 4 or 5.

    Here's a small example of building a link list module::

        from admin_tools.dashboard import modules, Dashboard

        class MyDashboard(Dashboard):
            def __init__(self, **kwargs):
                Dashboard.__init__(self, **kwargs)

                self.children.append(modules.LinkList(
                    layout='inline',
                    children=(
                        {
                            'title': 'Python website',
                            'url': 'http://www.python.org',
                            'external': True,
                            'description': 'Python language rocks !',
                            'attrs': {'target': '_blank'},
                        },
                        ['Django', 'http://www.djangoproject.com', True],
                        ['Some internal link', '/some/internal/link/'],
                    )
                ))

    The screenshot of what this code produces:

    .. image:: images/linklist_dashboard_module.png
    """

    title = _('Links')
    template = 'admin_tools/dashboard/modules/link_list.html'
    layout = 'stacked'

    def init_with_context(self, context):
        if self._initialized:
            return
        new_children = []
        for link in self.children:
            if isinstance(
                link,
                (
                    tuple,
                    list,
                ),
            ):
                link_dict = {'title': link[0], 'url': link[1]}
                if len(link) >= 3:
                    link_dict['external'] = link[2]
                if len(link) >= 4:
                    link_dict['description'] = link[3]
                if len(link) >= 5:
                    link_dict['attrs'] = link[4]
                link = link_dict
            if 'attrs' not in link:
                link['attrs'] = {}
            link['attrs']['href'] = link['url']
            if link.get('description', ''):
                link['attrs']['title'] = link['description']
            if link.get('external', False):
                link['attrs']['class'] = ' '.join(['external-link'] + link['attrs'].get('class', '').split()).strip()
            link['attrs'] = flatatt(link['attrs'])
            new_children.append(link)
        self.children = new_children
        self._initialized = True


class AppList(DashboardModule, AppListElementMixin):
    """
    Module that lists installed apps and their models.
    As well as the :class:`~admin_tools.dashboard.modules.DashboardModule`
    properties, the :class:`~admin_tools.dashboard.modules.AppList`
    has two extra properties:

    ``models``
        A list of models to include, only models whose name (e.g.
        "blog.comments.models.Comment") match one of the strings (e.g.
        "blog.*") in the models list will appear in the dashboard module.

    ``exclude``
        A list of models to exclude, if a model name (e.g.
        "blog.comments.models.Comment") match an element of this list (e.g.
        "blog.comments.*") it won't appear in the dashboard module.

    If no models/exclude list is provided, **all apps** are shown.

    Here's a small example of building an app list module::

        from admin_tools.dashboard import modules, Dashboard

        class MyDashboard(Dashboard):
            def __init__(self, **kwargs):
                Dashboard.__init__(self, **kwargs)

                # will only list the django.contrib apps
                self.children.append(modules.AppList(
                    title='Administration',
                    models=('django.contrib.*',)
                ))
                # will list all apps except the django.contrib ones
                self.children.append(modules.AppList(
                    title='Applications',
                    exclude=('django.contrib.*',)
                ))

    The screenshot of what this code produces:

    .. image:: images/applist_dashboard_module.png

    .. note::

        Note that this module takes into account user permissions, for
        example, if a user has no rights to change or add a ``Group``, then
        the django.contrib.auth.Group model line will not be displayed.
    """

    title = _('Applications')
    template = 'admin_tools/dashboard/modules/app_list.html'
    models = None
    exclude = None
    include_list = None
    exclude_list = None

    def __init__(self, title=None, **kwargs):
        self.models = list(kwargs.pop('models', []))
        self.exclude = list(kwargs.pop('exclude', []))
        self.include_list = kwargs.pop('include_list', [])  # deprecated
        self.exclude_list = kwargs.pop('exclude_list', [])  # deprecated
        super(AppList, self).__init__(title, **kwargs)

    def init_with_context(self, context):
        if self._initialized:
            return
        items = self._visible_models(context['request'])
        apps = {}
        for model, perms in items:
            app_label = model._meta.app_label
            if app_label not in apps:
                apps[app_label] = {
                    'title': django_apps.get_app_config(app_label).verbose_name,
                    'url': self._get_admin_app_list_url(model, context),
                    'models': [],
                }
            model_dict = {}
            model_dict['title'] = model._meta.verbose_name_plural
            if perms['change'] or perms.get('view', False):
                model_dict['change_url'] = self._get_admin_change_url(model, context)
            if perms['add']:
                model_dict['add_url'] = self._get_admin_add_url(model, context)
            apps[app_label]['models'].append(model_dict)

        for app in sorted(apps.keys()):
            # sort model list alphabetically
            apps[app]['models'].sort(key=lambda x: x['title'])
            self.children.append(apps[app])
        self._initialized = True


class ModelList(DashboardModule, AppListElementMixin):
    """
    Module that lists a set of models.
    As well as the :class:`~admin_tools.dashboard.modules.DashboardModule`
    properties, the :class:`~admin_tools.dashboard.modules.ModelList` takes
    two extra arguments:

    ``models``
        A list of models to include, only models whose name (e.g.
        "blog.comments.models.Comment") match one of the strings (e.g.
        "blog.*") in the models list will appear in the dashboard module.

    ``exclude``
        A list of models to exclude, if a model name (e.g.
        "blog.comments.models.Comment") match an element of this list (e.g.
        "blog.comments.*") it won't appear in the dashboard module.

    Here's a small example of building a model list module::

        from admin_tools.dashboard import modules, Dashboard

        class MyDashboard(Dashboard):
            def __init__(self, **kwargs):
                Dashboard.__init__(self, **kwargs)

                # will only list the django.contrib.auth models
                self.children += [
                    modules.ModelList(
                        title='Authentication',
                        models=['django.contrib.auth.*',]
                    )
                ]

    The screenshot of what this code produces:

    .. image:: images/modellist_dashboard_module.png

    .. note::

        Note that this module takes into account user permissions, for
        example, if a user has no rights to change or add a ``Group``, then
        the django.contrib.auth.Group model line will not be displayed.
    """

    template = 'admin_tools/dashboard/modules/model_list.html'
    models = None
    exclude = None
    include_list = None
    exclude_list = None

    def __init__(self, title=None, models=None, exclude=None, **kwargs):
        self.models = list(models or [])
        self.exclude = list(exclude or [])
        self.include_list = kwargs.pop('include_list', [])  # deprecated
        self.exclude_list = kwargs.pop('exclude_list', [])  # deprecated
        if 'extra' in kwargs:
            self.extra = kwargs.pop('extra')
        else:
            self.extra = []
        super(ModelList, self).__init__(title, **kwargs)

    def init_with_context(self, context):
        if self._initialized:
            return
        items = self._visible_models(context['request'])
        if not items:
            return
        for model, perms in items:
            model_dict = {}
            model_dict['title'] = model._meta.verbose_name_plural
            if perms['change'] or perms.get('view', False):
                model_dict['change_url'] = self._get_admin_change_url(model, context)
            if perms['add']:
                model_dict['add_url'] = self._get_admin_add_url(model, context)
            self.children.append(model_dict)
        if self.extra:
            for extra_url in self.extra:
                model_dict = {}
                model_dict['title'] = extra_url['title']
                model_dict['change_url'] = extra_url['change_url']
                model_dict['add_url'] = extra_url.get('add_url', None)
                self.children.append(model_dict)

        self._initialized = True


class Feed(DashboardModule):
    """
    Class that represents a feed dashboard module.

    .. important::

        This class uses the
        `Universal Feed Parser module <http://www.feedparser.org/>`_ to parse
        the feeds, so you'll need to install it, all feeds supported by
        FeedParser are thus supported by the Feed

    As well as the :class:`~admin_tools.dashboard.modules.DashboardModule`
    properties, the :class:`~admin_tools.dashboard.modules.Feed` takes two
    extra keyword arguments:

    ``feed_url``
        The URL of the feed.

    ``limit``
        The maximum number of feed children to display. Default value: None,
        which means that all children are displayed.

    Here's a small example of building a recent actions module::

        from admin_tools.dashboard import modules, Dashboard

        class MyDashboard(Dashboard):
            def __init__(self, **kwargs):
                Dashboard.__init__(self, **kwargs)

                # will only list the django.contrib apps
                self.children.append(modules.Feed(
                    title=_('Latest Django News'),
                    feed_url='http://www.djangoproject.com/rss/weblog/',
                    limit=5
                ))

    The screenshot of what this code produces:

    .. image:: images/feed_dashboard_module.png
    """

    title = _('RSS Feed')
    template = 'admin_tools/dashboard/modules/feed.html'
    feed_url = None
    limit = None

    def __init__(self, title=None, feed_url=None, limit=None, **kwargs):
        kwargs.update({'feed_url': feed_url, 'limit': limit})
        super(Feed, self).__init__(title, **kwargs)

    def init_with_context(self, context):
        if self._initialized:
            return
        import datetime

        if self.feed_url is None:
            raise ValueError('You must provide a valid feed URL')
        try:
            import feedparser
        except ImportError:
            self.children.append(
                {
                    'title': ('You must install the FeedParser python module'),
                    'warning': True,
                }
            )
            return

        feed = feedparser.parse(self.feed_url)
        if self.limit is not None:
            entries = feed['entries'][: self.limit]
        else:
            entries = feed['entries']
        for entry in entries:
            entry.url = entry.link
            try:
                entry.date = datetime.date(*entry.published_parsed[0:3])
            except:
                # no date for certain feeds
                pass
