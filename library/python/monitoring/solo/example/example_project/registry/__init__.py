import itertools

from library.python.monitoring.solo.example.example_project.registry.alert import get_all_alerts
from library.python.monitoring.solo.example.example_project.registry.channel import get_all_channels
from library.python.monitoring.solo.example.example_project.registry.cluster import get_all_clusters
from library.python.monitoring.solo.example.example_project.registry.dashboard import get_all_dashboards
from library.python.monitoring.solo.example.example_project.registry.graph import get_all_graphs
from library.python.monitoring.solo.example.example_project.registry.menu import get_all_menu
from library.python.monitoring.solo.example.example_project.registry.service import get_all_services
from library.python.monitoring.solo.example.example_project.registry.shard import get_all_shards

# импортируем все объекты Соло и собираем в один общий список registry
# потом registry подается на вход контроллерам Solomon и Juggler
objects_registry = list(itertools.chain(
    get_all_clusters(),
    get_all_services(),
    get_all_shards(),
    get_all_alerts(),
    get_all_channels(),
    get_all_graphs(),
    get_all_dashboards(),
    get_all_menu(),
))
