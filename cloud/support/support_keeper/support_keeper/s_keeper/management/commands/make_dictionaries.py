from s_keeper.externals import get_components, get_ranks

from django.core.management import BaseCommand
from s_keeper.models import ComponentsDict, RanksDict
from s_keeper.serializers import ComponentsDictSerializer, RanksDictSerializer
from uuid import uuid4

class Command(BaseCommand):
    def handle(self, *args, **options):
        ComponentsDict.objects.all().delete()
        for component in get_components():
            data = {
                'u_id': uuid4(),
                'name': component['name'],
                'c_id': component['id']
            }
            new_component = ComponentsDict(**data)
            new_component_srlzr = ComponentsDictSerializer(data=data)
            new_component_srlzr.is_valid()
            new_component.save(new_component_srlzr.validated_data)

        RanksDict.objects.all().delete()
        for rank in get_ranks():
            data = {
                'name': rank
            }
            new_rank = RanksDict(**data)
            new_rank_srlzr = RanksDictSerializer(data=data)
            new_rank_srlzr.is_valid()
            new_rank.save(new_component_srlzr.validated_data)
