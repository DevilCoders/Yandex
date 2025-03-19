import django.db

import cloud.mdb.backstage.lib.helpers as mod_helpers


class ScheduleState:
    ACTIVE = ('active', 'Active')
    STOPPED = ('stopped', 'Stopped')
    BROKEN = ('broken', 'Broken')

    all = [
        ACTIVE,
        STOPPED,
        BROKEN,
    ]


class ClusterRolloutState:
    PENDING = ('pending', 'Pending')
    RUNNING = ('running', 'Running')
    SUCCEEDED = ('succeeded', 'Succeeded')
    CANCELLED = ('cancelled', 'Cancelled')
    SKIPPED = ('skipped', 'Skipped')
    FAILED = ('failed', 'Failed')

    all = [
        PENDING,
        RUNNING,
        SUCCEEDED,
        CANCELLED,
        SKIPPED,
        FAILED,
    ]


class ClusterRolloutUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('rollout')\
            .select_related('cluster')\
            .order_by('-updated_at')


class ClusterRollout(django.db.models.Model):
    rollout = django.db.models.ForeignKey('Rollout', django.db.models.DO_NOTHING)
    cluster = django.db.models.ForeignKey('Cluster', django.db.models.DO_NOTHING)
    state = django.db.models.CharField(max_length=16, choices=ClusterRolloutState.all, primary_key=True)  # fake primary key for ORM works
    updated_at = django.db.models.DateTimeField()
    comment = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_list = ClusterRolloutUiListManager()

    class Meta:
        managed = False
        db_table = 'cluster_rollouts'
        unique_together = (('rollout', 'cluster'),)


class ClusterUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('-imported_at')


class ClusterUiObjManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('host_set')


class Cluster(django.db.models.Model, mod_helpers.LinkedMixin):
    cluster_id = django.db.models.TextField(primary_key=True)
    tags = django.db.models.JSONField()
    imported_at = django.db.models.DateTimeField()
    auto_update = django.db.models.BooleanField()

    objects = django.db.models.Manager()
    ui_obj = ClusterUiObjManager()
    ui_list = ClusterUiListManager()

    class Meta:
        managed = False
        db_table = 'clusters'

    def __str__(self):
        return self.cluster_id


class HostUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')


class HostUiObjManager(django.db.models.Manager):
    pass


class Host(django.db.models.Model, mod_helpers.LinkedMixin):
    fqdn = django.db.models.TextField(primary_key=True)
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING)
    tags = django.db.models.JSONField()

    objects = django.db.models.Manager()
    ui_obj = HostUiObjManager()
    ui_list = HostUiListManager()

    class Meta:
        managed = False
        db_table = 'hosts'

    def __str__(self):
        return self.fqdn


class RolloutShipment(django.db.models.Model):
    rollout = django.db.models.ForeignKey('Rollout', django.db.models.DO_NOTHING)
    fqdn = django.db.models.TextField()
    shipment_id = django.db.models.BigIntegerField(primary_key=True)  # fake primary key for ORM works

    class Meta:
        managed = False
        db_table = 'rollout_shipments'
        unique_together = (('rollout', 'fqdn', 'shipment_id'),)

    def __str__(self):
        return str(self.rollout)


class ScheduleDependency(django.db.models.Model):
    schedule = django.db.models.ForeignKey('Schedule', django.db.models.DO_NOTHING)
    depends_on = django.db.models.ForeignKey('Schedule', django.db.models.DO_NOTHING, related_name='depended')

    class Meta:
        managed = False
        db_table = 'schedule_dependencies'
        unique_together = (('schedule', 'depends_on'),)


class ScheduleUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('-edited_at')


class ScheduleUiObjManager(ScheduleUiListManager):
    pass


class Schedule(django.db.models.Model, mod_helpers.LinkedMixin):
    schedule_id = django.db.models.BigIntegerField(primary_key=True)
    match_tags = django.db.models.JSONField()
    commands = django.db.models.JSONField()
    state = django.db.models.CharField(max_length=8, choices=ScheduleState.all)
    age = django.db.models.DurationField()
    still_age = django.db.models.DurationField()
    max_size = django.db.models.BigIntegerField()
    parallel = django.db.models.IntegerField()
    edited_at = django.db.models.DateTimeField()
    edited_by = django.db.models.TextField(blank=True, null=True)
    examined_rollout_id = django.db.models.BigIntegerField(blank=True, null=True)
    namespace = django.db.models.TextField()
    name = django.db.models.TextField()

    objects = django.db.models.Manager()
    ui_obj = ScheduleUiObjManager()
    ui_list = ScheduleUiListManager()

    class Meta:
        managed = False
        db_table = 'schedules'

    def __str__(self):
        return str(self.pk)

    @property
    def last_rollout_id(self):
        r = Rollout.objects.filter(schedule=self).order_by('-created_at').first()
        if r:
            return r.rollout_id

    def get_convergence(self):
        with django.db.connections['katan_db'].cursor() as cursor:
            cursor.execute(
                """
                SELECT min(last_succeeded_age) AS min_age,
                         max(last_succeeded_age) AS max_age,
                         count(*) count,
                         (array_agg(cluster_id))[:3] sample_clusters
                     FROM (
                    SELECT cluster_id,
                           date_trunc('day', age(max(cr.updated_at)) + interval '1 day') AS last_succeeded_age
                      FROM rollouts r
                      JOIN cluster_rollouts cr USING (rollout_id)
                     WHERE cr.state = 'succeeded'
                       AND schedule_id = %(schedule_id)s
                     GROUP BY cluster_id
                  ) x
                GROUP BY width_bucket(last_succeeded_age, '{5 days,14 days,30 days}'::interval[])
                UNION
                SELECT NULL,
                       NULL,
                       count(*),
                       (array_agg(cluster_id))[:3] sample_clusters
                  FROM schedules s
                  JOIN clusters c
                    ON (c.tags @> s.match_tags)
                WHERE s.schedule_id = %(schedule_id)s
                  AND NOT EXISTS (
                    SELECT 1
                      FROM rollouts r
                      JOIN cluster_rollouts cr USING (rollout_id)
                     WHERE cr.state = 'succeeded'
                       AND r.schedule_id = s.schedule_id
                  )
                ORDER BY 1 NULLS LAST
            """,
                {'schedule_id': self.schedule_id},
            )
            return cursor.fetchall()


class RolloutUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('schedule')\
            .order_by('-created_at')


class RolloutUiObjManager(RolloutUiListManager):
    pass


class Rollout(django.db.models.Model, mod_helpers.LinkedMixin):
    rollout_id = django.db.models.BigIntegerField(primary_key=True)
    commands = django.db.models.JSONField()
    parallel = django.db.models.IntegerField()
    created_at = django.db.models.DateTimeField()
    started_at = django.db.models.DateTimeField(blank=True, null=True)
    finished_at = django.db.models.DateTimeField(blank=True, null=True)
    created_by = django.db.models.TextField(blank=True, null=True)
    schedule = django.db.models.ForeignKey(Schedule, django.db.models.DO_NOTHING, blank=True, null=True)
    comment = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = RolloutUiObjManager()
    ui_list = RolloutUiListManager()

    class Meta:
        managed = False
        db_table = 'rollouts'

    def __str__(self):
        return str(self.rollout_id)


class RolloutsDependency(django.db.models.Model):
    rollout = django.db.models.ForeignKey(Rollout, django.db.models.DO_NOTHING)
    depends_on = django.db.models.ForeignKey(Rollout, django.db.models.DO_NOTHING, related_name='depended')

    class Meta:
        managed = False
        db_table = 'rollouts_dependencies'
        unique_together = (('rollout', 'depends_on'),)
