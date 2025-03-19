from django.contrib.postgres.fields import JSONField
from django.db import models, connections

from cloud.mdb.ui.internal.common import LinkedMixin

SCHEDULE_STATES = [
    ('active', 'active'),
    ('stopped', 'stopped'),
    ('broken', 'broken'),
]

CLUSTER_ROLLOUT_STATES = [
    ('pending', 'pending'),
    ('running', 'running'),
    ('succeeded', 'succeeded'),
    ('cancelled', 'cancelled'),
    ('skipped', 'skipped'),
    ('failed', 'failed'),
]


class ClusterRollout(models.Model):
    rollout = models.ForeignKey('Rollout', models.DO_NOTHING)
    cluster = models.ForeignKey('Cluster', models.DO_NOTHING)
    state = models.CharField(max_length=16, choices=CLUSTER_ROLLOUT_STATES)
    updated_at = models.DateTimeField()
    comment = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'cluster_rollouts'
        unique_together = (('rollout', 'cluster'),)


class Cluster(models.Model, LinkedMixin):
    cluster_id = models.TextField(primary_key=True)
    tags = JSONField()
    imported_at = models.DateTimeField()
    auto_update = models.BooleanField()

    class Meta:
        managed = False
        db_table = 'clusters'

    def __str__(self):
        return self.cluster_id


class Host(models.Model, LinkedMixin):
    fqdn = models.TextField(primary_key=True)
    cluster = models.ForeignKey(Cluster, models.DO_NOTHING)
    tags = JSONField()

    class Meta:
        managed = False
        db_table = 'hosts'

    def __str__(self):
        return self.fqdn


class RolloutShipment(models.Model):
    rollout = models.ForeignKey('Rollout', models.DO_NOTHING)
    fqdn = models.TextField()
    shipment_id = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'rollout_shipments'
        unique_together = (('rollout', 'fqdn', 'shipment_id'),)

    def __str__(self):
        return str(self.rollout)


class Rollout(models.Model, LinkedMixin):
    rollout_id = models.BigIntegerField(primary_key=True)
    commands = JSONField()
    parallel = models.IntegerField()
    created_at = models.DateTimeField()
    started_at = models.DateTimeField(blank=True, null=True)
    finished_at = models.DateTimeField(blank=True, null=True)
    created_by = models.TextField(blank=True, null=True)
    schedule = models.ForeignKey('Schedule', models.DO_NOTHING, blank=True, null=True)
    comment = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'rollouts'

    def __str__(self):
        return str(self.rollout_id)


class RolloutsDependency(models.Model):
    rollout = models.ForeignKey(Rollout, models.DO_NOTHING)
    depends_on = models.ForeignKey(Rollout, models.DO_NOTHING, related_name='depended')

    class Meta:
        managed = False
        db_table = 'rollouts_dependencies'
        unique_together = (('rollout', 'depends_on'),)


class ScheduleDependency(models.Model):
    schedule = models.ForeignKey('Schedule', models.DO_NOTHING)
    depends_on = models.ForeignKey('Schedule', models.DO_NOTHING, related_name='depended')

    class Meta:
        managed = False
        db_table = 'schedule_dependencies'
        unique_together = (('schedule', 'depends_on'),)


class Schedule(models.Model):
    schedule_id = models.BigIntegerField(primary_key=True)
    match_tags = JSONField()
    commands = JSONField()
    state = models.CharField(max_length=8, choices=SCHEDULE_STATES)
    age = models.DurationField()
    still_age = models.DurationField()
    max_size = models.BigIntegerField()
    parallel = models.IntegerField()
    edited_at = models.DateTimeField()
    edited_by = models.TextField(blank=True, null=True)
    examined_rollout_id = models.BigIntegerField(blank=True, null=True)
    namespace = models.TextField()
    name = models.TextField()

    class Meta:
        managed = False
        db_table = 'schedules'

    def __str__(self):
        return str(self.schedule_id)

    @property
    def last_rollout_id(self):
        r = Rollout.objects.filter(schedule=self).order_by('-created_at').first()
        if r:
            return r.rollout_id

    def get_convergence(self):
        with connections['katan_slave'].cursor() as cursor:
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
