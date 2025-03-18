# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import django.dispatch


# Arguments: "login", "group", "role", "fields", "org_id", "uid"
role_added = django.dispatch.Signal()
# Arguments: "login", "group", "role", "data", "is_fired", "is_deleted", "org_id"
role_removed = django.dispatch.Signal()
# Arguments: "login", "group", "role", "fields", "org_id"
tvm_role_added = django.dispatch.Signal()
# Arguments: "login", "group", "role", "fields", "org_id"
org_role_added = django.dispatch.Signal()
# Arguments: "login", "group", "role", "data", "is_fired", "is_deleted", "org_id"
tvm_role_removed = django.dispatch.Signal()
# Arguments: "login", "group", "role", "data", "is_fired", "is_deleted", "org_id"
org_role_removed = django.dispatch.Signal()
# Arguments: "login", "group_external_id", "passport_login", "org_id"
membership_added = django.dispatch.Signal()
# Arguments: "login", "group_external_id", "passport_login", "org_id"
membership_removed = django.dispatch.Signal()
