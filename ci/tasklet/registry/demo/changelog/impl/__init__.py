from tasklet.services.ci import get_ci_env
from ci.tasklet.common.proto import service_pb2
from ci.tasklet.registry.demo.changelog.proto import schema_tasklet


class ChangelogImpl(schema_tasklet.ChangelogBase):
    LIMIT = 300

    def run(self):
        request = service_pb2.GetCommitsRequest()
        # извлекаем id запуска из контекста
        request.flow_launch_id = self.input.context.job_instance_id.flow_launch_id
        request.limit = ChangelogImpl.LIMIT  # по умолчанию установлен лимит в 300 коммитов
        request.ci_env = get_ci_env(self.input.context)

        changelog = []
        issues = []
        hashes = []
        response = self.ctx.ci.GetCommits(request)
        for c in response.commits:
            changelog += [self._format(c)]
            issues += c.issues
            hashes += [c.revision.hash]

        if response.HasField('next'):
            # можно передать значение поля next из ответа в следующий запрос (поле offset)
            # для получения следующей пачки коммитов
            changelog.append('...truncated...')

        self.output.changelog.content.extend(changelog)
        self.output.changelog.issues.extend(issues)
        self.output.changelog.hashes.extend(hashes)

    def _format(self, commit):
        revision = ''
        if commit.revision.hash:
            revision = f'((https://a.yandex-team.ru/arc_vcs/commit/{commit.revision.hash} {commit.revision.hash[:10]}))'
        else:
            revision = f'((https://a.yandex-team.ru/arc/commit/{commit.revision.number} {commit.revision.number}))'

        # отрезается лишнее
        # в message приходит и описание пулл-реквеста, в котором бывает лишний мусор: ссылки на беты, роботные отбивки и пр.)
        # https://jing.yandex-team.ru/files/baymer/uhura_2021-08-04T19%3A00%3A15.839743.jpg
        message = commit.message.rsplit('\n')[0]

        return f'{revision}: {message}'
