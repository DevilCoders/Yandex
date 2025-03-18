import jsonobject

from library.python.monitoring.solo.objects.juggler.base import JugglerObject


class Link(jsonobject.JsonObject):
    title = jsonobject.StringProperty()
    url = jsonobject.StringProperty()


class Filter(jsonobject.JsonObject):
    host = jsonobject.StringProperty(default="")
    service = jsonobject.StringProperty(default="")
    namespace = jsonobject.StringProperty(default="")
    project = jsonobject.StringProperty(default="")
    tags = jsonobject.ListProperty(str)
    recipients = jsonobject.ListProperty(str)
    responsibles = jsonobject.ListProperty(str)


class Sort(jsonobject.JsonObject):
    field = jsonobject.StringProperty(default="DEFAULT")
    order = jsonobject.StringProperty(default="DESC", choices=["DESC", "ASC"])


class NotificationFilters(jsonobject.JsonObject):
    host = jsonobject.StringProperty(default="")
    service = jsonobject.StringProperty(default="")
    login = jsonobject.StringProperty(default="")
    method = jsonobject.StringProperty(default="")
    status = jsonobject.StringProperty(default="")


class DashboardNotificationOptions(jsonobject.JsonObject):
    filters = jsonobject.ListProperty(NotificationFilters)
    interval = jsonobject.IntegerProperty(default=604800)
    page_size = jsonobject.IntegerProperty(default=0)


class MutesFilters(jsonobject.JsonObject):
    check_id = jsonobject.StringProperty(default="")
    host = jsonobject.StringProperty(default="")
    login = jsonobject.StringProperty(default="")
    service = jsonobject.StringProperty(default="")
    method = jsonobject.StringProperty(default="")
    mute_id = jsonobject.StringProperty(default="")
    namespace = jsonobject.StringProperty(default="")
    project = jsonobject.StringProperty(default="")
    user = jsonobject.StringProperty(default="")
    tags = jsonobject.ListProperty(str)


class MutesOptions(jsonobject.JsonObject):
    filters = jsonobject.ListProperty(MutesFilters)
    include_expired = jsonobject.BooleanProperty(default=False)
    project = jsonobject.StringProperty(default="")
    page = jsonobject.IntegerProperty(default=0)
    page_size = jsonobject.IntegerProperty(default=0)
    sort_by = jsonobject.StringProperty(default="ID")
    sort_order = jsonobject.StringProperty(default="DESC")


class DowntimeFilter(jsonobject.JsonObject):
    downtime_id = jsonobject.StringProperty(default="")
    host = jsonobject.StringProperty(default="")
    instance = jsonobject.StringProperty(default="")
    namespace = jsonobject.StringProperty(default="")
    project = jsonobject.StringProperty(default="")
    source = jsonobject.StringProperty(default="")
    user = jsonobject.StringProperty(default="")
    tags = jsonobject.ListProperty(str)


class DowntimesOptions(jsonobject.JsonObject):
    filters = jsonobject.ListProperty(DowntimeFilter)
    include_expired = jsonobject.BooleanProperty(default=False)
    include_warnings = jsonobject.BooleanProperty(default=False)
    exclude_future = jsonobject.BooleanProperty(default=False)
    project = jsonobject.StringProperty(default="")
    page = jsonobject.IntegerProperty(default=0)
    page_size = jsonobject.IntegerProperty(default=0)
    sort_by = jsonobject.StringProperty(default="ID")
    sort_order = jsonobject.StringProperty(default="DESC")


class ChecksOptions(jsonobject.JsonObject):
    project = jsonobject.StringProperty(default="")
    filters = jsonobject.ListProperty(Filter)
    include_mutes = jsonobject.BooleanProperty(default=False)
    limit = jsonobject.IntegerProperty(default=0)
    statuses = jsonobject.ListProperty(jsonobject.StringProperty(
        choices=["CRIT", "WARN", "OK"]
    ), default=["CRIT", "WARN", "OK"])
    sort = jsonobject.ObjectProperty(Sort)


class IframeOptions(jsonobject.JsonObject):
    url = jsonobject.StringProperty()


class Component(jsonobject.JsonObject):
    name = jsonobject.StringProperty(required=True)
    links = jsonobject.ListProperty(Link)
    column = jsonobject.IntegerProperty(name="col", default=0)
    row = jsonobject.IntegerProperty(name="row", default=0)
    rowspan = jsonobject.IntegerProperty(default=0)
    colspan = jsonobject.IntegerProperty(default=0)
    elements_in_row = jsonobject.IntegerProperty(default=1)
    aggregate_checks_options = jsonobject.ObjectProperty(ChecksOptions, default=None, exclude_if_none=True)
    raw_events_options = jsonobject.ObjectProperty(ChecksOptions, default=None, exclude_if_none=True)
    notifications_options = jsonobject.ObjectProperty(DashboardNotificationOptions, default=None, exclude_if_none=True)
    mutes_options = jsonobject.ObjectProperty(MutesOptions, default=None, exclude_if_none=True)
    downtimes_options = jsonobject.ObjectProperty(DowntimesOptions, default=None, exclude_if_none=True)
    iframe_options = jsonobject.ObjectProperty(IframeOptions, default=None, exclude_if_none=True)
    height_px = jsonobject.IntegerProperty(default=200)
    width_px = jsonobject.IntegerProperty(default=200)
    view_type = jsonobject.StringProperty(
        choices=[
            "COMPACT",
            "DETAILED",
        ],
        default="COMPACT",
    )
    component_type = jsonobject.StringProperty(
        choices=[
            "AGGREGATE_CHECKS",
            "RAW_EVENTS",
            "IFRAME",
            "NOTIFICATIONS",
            "MUTES",
            "DOWNTIMES"
        ]
    )


class Dashboard(JugglerObject):
    __OBJECT_TYPE__ = "Dashboard"

    address = jsonobject.StringProperty(required=True)
    dashboard_id = jsonobject.StringProperty(exclude_if_none=True)
    name = jsonobject.StringProperty(required=True)
    description = jsonobject.StringProperty()
    project = jsonobject.StringProperty(default="")
    owners = jsonobject.ListProperty(str)
    links = jsonobject.ListProperty(Link)
    components = jsonobject.ListProperty(Component)
