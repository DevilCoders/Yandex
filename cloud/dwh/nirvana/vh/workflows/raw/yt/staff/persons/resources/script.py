import json
import nirvana.job_context as nv
import requests


class QueryBuilder:

    def __init__(self, table_path):
        self._table_path = table_path

    def create_query(self):
        q = """
    CREATE TABLE IF NOT EXISTS `{table_path}`
    (
        firstname_en            String,
        firstname_ru            String,
        lastname_en             String,
        id                      Int32,
        is_deleted              String,
        lastname_ru             String,
        login                   String,
        official_affiliation    String,
        official_is_dismissed   String,
        official_is_homeworker  String,
        official_is_robot       String,
        official_quit_at        String,
        uid                     String,
        yandex_login            String,
        department_ancestors    String,
        department              String,
        load_ts                 DateTime
    ) ENGINE = YtTable();
        """.format(table_path=self._table_path)

        return q

    def drop_query(self):
        q = """
    DROP TABLE IF EXISTS `{table_path}`;
        """.format(table_path=self._table_path)

        return q

    def insert_query(self, persons):
        q = """
                INSERT INTO `{table_path}` (
                        firstname_en,
                        firstname_ru,
                        lastname_en,
                        id,
                        is_deleted,
                        lastname_ru,
                        login,
                        official_affiliation,
                        official_is_dismissed,
                        official_is_homeworker,
                        official_is_robot,
                        official_quit_at,
                        uid,
                        yandex_login,
                        department_ancestors,
                        department,
                        load_ts
                    )
                VALUES
            """.format(table_path=self._table_path)

        for i, person in enumerate(persons):
            separator = ',' if i else ''

            q = """{base_q}
                {separator}(
                        '{firstname_en}',
                        '{firstname_ru}',
                        '{lastname_en}',
                        {id},
                        '{is_deleted}',
                        '{lastname_ru}',
                        '{login}',
                        '{official_affiliation}',
                        '{official_is_dismissed}',
                        '{official_is_homeworker}',
                        '{official_is_robot}',
                        '{official_quit_at}',
                        '{uid}',
                        '{yandex_login}',
                        '{department_ancestors}',
                        '{department}',
                        now()
                )
            """.format(base_q=q,
                       firstname_en=person.firstname_en,
                       firstname_ru=person.firstname_ru,
                       lastname_en=person.lastname_en,
                       id=person.id,
                       is_deleted=person.is_deleted,
                       lastname_ru=person.lastname_ru,
                       login=person.login,
                       official_affiliation=person.official_affiliation,
                       official_is_dismissed=person.official_is_dismissed,
                       official_is_homeworker=person.official_is_homeworker,
                       official_is_robot=person.official_is_robot,
                       official_quit_at=person.official_quit_at,
                       uid=person.uid,
                       yandex_login=person.yandex_login,
                       department_ancestors=person.department_ancestors,
                       department=person.department,
                       separator=separator)

        return q


class QueryExecutor:
    def __init__(self, yt_cluster, yt_token, chyt_alias="*ch_public", timeout=600):
        self._yt_cluster = yt_cluster
        self._yt_token = yt_token
        self._chyt_alias = chyt_alias
        self._default_timeout = timeout

    def execute(self, query, timeout=None):
        proxy = "http://{cluster}.yt.yandex.net".format(cluster=self._yt_cluster)
        s = requests.Session()

        url = "{proxy}/query?database={alias}&password={token}".format(
            proxy=proxy,
            alias=self._chyt_alias,
            token=self._yt_token)

        resp = s.post(url, data=query, timeout=timeout or self._default_timeout)
        if resp.status_code != 200:
            print("Request payload: %s", query)
            print("Query response status: %s", resp.status_code)
            print("Query response headers: %s", resp.headers)
            print("Query response content: %s", resp.content)

        resp.raise_for_status()

        rows = resp.content.strip()
        return rows


class StaffLoader:
    CLOUD_ANCESTOR_ID = 70618
    CLOUD_DEPARTMENT_ID = 4505
    ROBOT_DEPARTMENT_ID = 2073

    def __init__(self, staff_url, staff_token):
        self._staff_url = staff_url
        self._staff_token = staff_token

    def _make_params(self, page):
        PAGE_SIZE = 1000

        department_ancestors = "department_group.ancestors.level,department_group.ancestors.id,department_group.ancestors.name,department_group.ancestors.url,department_group.ancestors.type"
        department = "department_group.department.level,department_group.department.id,department_group.department.name,department_group.department.url,department_group.department.type"

        # filters = 'department_group.ancestors.id=={anc_id} or department_group.department.id=={cloud_dep_id}'
        filters = 'department_group.ancestors.id=={anc_id} or department_group.department.id in [{cloud_dep_id},{robot_dep_id}]'

        filter_query = filters.format(
            anc_id=str(self.CLOUD_ANCESTOR_ID),
            cloud_dep_id=str(self.CLOUD_DEPARTMENT_ID),
            robot_dep_id=str(self.ROBOT_DEPARTMENT_ID)
        )

        params = {
            "_limit": PAGE_SIZE,
            "_page": page,
            '_sort': 'id',
            '_query': filter_query,
            '_fields': department + ',' + department_ancestors + ',created_at,yandex,official,login,id,name,guid,_meta'
        }
        return params

    def get_page(self, page):
        headers = {'Authorization': 'OAuth ' + self._staff_token}
        params = self._make_params(page)

        with requests.Session() as s:
            response = s.get(self._staff_url, headers=headers, params=params)
            if response.status_code != 200:
                print("Requst params:" + str(params))
                print("Response:" + response.content)
                response.raise_for_status()
            else:
                return json.loads(response.text)


class PersonParser:

    def parse_json(self, persons_json):
        persons = []
        for person_json in persons_json:
            person = self.get_person(person_json)
            persons.append(person)
        return persons

    def get_person(self, person_json):
        firstname_en = self._replace_quotation(
            self._get(self._get(self._get(person_json, "name"), "first"), "en")).encode('utf-8').strip()
        firstname_ru = self._replace_quotation(
            self._get(self._get(self._get(person_json, "name"), "first"), "ru")).encode('utf-8').strip()
        lastname_en = self._replace_quotation(
            self._get(self._get(self._get(person_json, "name"), "last"), "en")).encode('utf-8').strip()
        lastname_ru = self._replace_quotation(
            self._get(self._get(self._get(person_json, "name"), "last"), "ru")).encode('utf-8').strip()
        id = self._get(person_json, "id")
        is_deleted = self._get(person_json, "is_deleted")
        login = self._get(person_json, "login")
        official_affiliation = self._get(self._get(person_json, "official"), "affiliation")
        official_is_dismissed = self._get(self._get(person_json, "official"), "is_dismissed")
        official_is_homeworker = self._get(self._get(person_json, "official"), "is_homeworker")
        official_is_robot = self._get(self._get(person_json, "official"), "is_robot")
        official_quit_at = self._get(self._get(person_json, "official"), "quit_at")
        uid = self._get(person_json, "guid")
        yandex_login = self._get(self._get(person_json, "yandex"), "login")
        department_group = self._get(person_json, "department_group")
        department_ancestors = json.dumps(self._get(department_group, "ancestors"))
        department = self._replace_quotation(json.dumps(self._get(department_group, "department"))).encode(
            'utf-8').strip()

        return Person(
            firstname_en,
            firstname_ru,
            lastname_en,
            lastname_ru,
            id,
            is_deleted,
            login,
            official_affiliation,
            official_is_dismissed,
            official_is_homeworker,
            official_is_robot,
            official_quit_at,
            uid,
            yandex_login,
            department_ancestors,
            department
        )

    def _get(self, source, key):
        if source is None or source == '':
            return ''
        return source.get(key)

    def _replace_quotation(self, source):
        return source.replace('\'', '\\\'').replace('\\\"', '\\\\\\"')


class Person:
    def __init__(self,
                 firstname_en,
                 firstname_ru,
                 lastname_en,
                 lastname_ru,
                 id,
                 is_deleted,
                 login,
                 official_affiliation,
                 official_is_dismissed,
                 official_is_homeworker,
                 official_is_robot,
                 official_quit_at,
                 uid,
                 yandex_login,
                 department_ancestors,
                 department
                 ):
        self.firstname_en = firstname_en
        self.firstname_ru = firstname_ru
        self.lastname_en = lastname_en
        self.lastname_ru = lastname_ru
        self.id = id
        self.is_deleted = is_deleted
        self.login = login
        self.official_affiliation = official_affiliation
        self.official_is_dismissed = official_is_dismissed
        self.official_is_homeworker = official_is_homeworker
        self.official_is_robot = official_is_robot
        self.official_quit_at = official_quit_at
        self.uid = uid
        self.yandex_login = yandex_login
        self.department_ancestors = department_ancestors
        self.department = department


def parse_parameters(parameters_text):
    param_s = parameters_text.split(";")
    params_dict = {}
    for param_string in param_s:
        split_param = param_string.split('=')
        params_dict[split_param[0]] = split_param[1]
    return params_dict


def read_secret_file(file_path):
    with open(file_path, 'r') as f:
        secret = f.readline().strip()
    return secret


def run_load_operation():
    input_filenames = nv.context().get_inputs().get_list('infiles')
    yt_token = read_secret_file(input_filenames[0])
    staff_token = read_secret_file(input_filenames[1])

    all_params = parse_parameters(nv.context().get_parameters().get('input-params'))
    staff_url = all_params.get("staff_api")
    yt_cluster = all_params.get("yt_cluster")
    dst_table_path = all_params.get("destination_path")

    query_builder = QueryBuilder(dst_table_path)
    query_executor = QueryExecutor(yt_cluster, yt_token)
    staff_loader = StaffLoader(staff_url, staff_token)
    parser = PersonParser()

    drop_table_query = query_builder.drop_query()
    query_executor.execute(drop_table_query)

    create_table_query = query_builder.create_query()
    query_executor.execute(create_table_query)

    i = 1
    page_limit = 1

    while i <= page_limit:
        persons_json = staff_loader.get_page(i)

        page_limit = persons_json["pages"]
        persons_json = persons_json["result"]

        persons = parser.parse_json(persons_json)
        q = query_builder.insert_query(persons)
        query_executor.execute(q)
        i += 1


def main():
    run_load_operation()


if __name__ == '__main__':
    main()
