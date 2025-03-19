from rest_framework import viewsets, views, status
from rest_framework.generics import mixins, RetrieveUpdateAPIView, get_object_or_404
from rest_framework.permissions import AllowAny, IsAuthenticated, IsAdminUser, IsAuthenticatedOrReadOnly
from .serializers import Queue_FilterSerializer, Support_UnitSerializer, ComponentsDictSerializer, RanksDictSerializer, \
    RegistrationSerializer, LoginSerializer, Support_UnitPendingSerializer
from .models import Queue_Filter, Support_Unit, ComponentsDict, RanksDict
from rest_framework.response import Response
from rest_framework.renderers import BrowsableAPIRenderer
from .renderers import UserJSONRenderer
from .backends import JWTAuthentication


class RegistrationAPIView(views.APIView):
    """
    only admins create new users
    """
    permission_classes = (IsAdminUser,)
    serializer_class = RegistrationSerializer
    renderer_classes = (UserJSONRenderer, BrowsableAPIRenderer,)

    def post(self, request):
        user = request.data.get('user', {})
        serializer = self.serializer_class(data=user)
        serializer.is_valid(raise_exception=True)
        serializer.save()
        return Response(serializer.data, status=status.HTTP_201_CREATED)


class LoginAPIView(views.APIView):
    permission_classes = (AllowAny,)
    renderer_classes = (UserJSONRenderer, BrowsableAPIRenderer,)
    serializer_class = LoginSerializer

    def post(self, request):
        user = request.data.get('user', {}) or request.data

        serializer = self.serializer_class(data=user)

        serializer.is_valid(raise_exception=True)

        return Response(serializer.data, status=status.HTTP_200_OK)


class UserRetrieveUpdateAPIView(RetrieveUpdateAPIView):
    permission_classes = (AllowAny,)
    renderer_classes = (UserJSONRenderer, BrowsableAPIRenderer)
    serializer_class = Support_UnitSerializer
    queryset = Support_Unit.objects.all()

    def retrieve(self, request, *args, **kwargs):
        serializer_data = request.query_params

        user = Support_Unit.objects.filter(login=serializer_data['login']).first()
        serializer = self.serializer_class(user)

        return Response(serializer.data, status=status.HTTP_200_OK)

    def update(self, request, *args, **kwargs):
        """
        may update only own info
        """
        serializer_data = request.data.get('user', {})

        if request.user.is_staff:
            user = Support_Unit.objects.filter(login=serializer_data['login']).first()
        else:
            user = Support_Unit.objects.filter(login=request.user).first()
        serializer = self.serializer_class(
             user, data=serializer_data, partial=True
        )
        serializer.is_valid(raise_exception=True)
        serializer.save()

        return Response(serializer.data, status=status.HTTP_200_OK)


class Support_UnitViewSet(viewsets.ModelViewSet):
    queryset = Support_Unit.objects.all()
    serializer_class = Support_UnitSerializer

    def get_queryset(self):
        return Support_Unit.objects.filter(is_absent=False)


class Queue_FilterViewSet(mixins.RetrieveModelMixin, mixins.ListModelMixin, mixins.CreateModelMixin,
                          mixins.UpdateModelMixin, mixins.DestroyModelMixin, viewsets.GenericViewSet):
    queryset = Queue_Filter.objects.all()
    serializer_class = Queue_FilterSerializer


class Support_UnitPendingViewSet(views.APIView):

    def get(self, request, *args, **kwargs):
        queryset = Support_Unit.objects.raw("""
                                           select * from
                (select sk1.queue_filter_id as new_qfid, sk1.support_unit_id as new_suid,
                sk2.support_unit_id as old_suid, sk2.queue_filter_id as old_qfid,
                        sksu.login as login, sksu.u_id as u_id, sk3.name as q_name
                from s_keeper_queue_filter_assignees as sk1
                left join s_keeper_queue_state as sk2
                on sk1.queue_filter_id = sk2.queue_filter_id and sk1.support_unit_id = sk2.support_unit_id
                left join s_keeper_support_unit sksu on sk1.support_unit_id = sksu.u_id
                left join s_keeper_queue_filter as sk3 on sk3.q_id = sk1.queue_filter_id
                union all
                select sk1.queue_filter_id as new_qfid, sk1.support_unit_id as new_suid,
                sk2.support_unit_id as old_suid, sk2.queue_filter_id as old_qfid,
                       sksu.login as login, sksu.u_id as u_id, sk3.name as q_name
                from s_keeper_queue_filter_assignees as sk1
                right join s_keeper_queue_state as sk2
                on sk1.queue_filter_id = sk2.queue_filter_id and sk1.support_unit_id = sk2.support_unit_id
                right join s_keeper_support_unit sksu on sk2.support_unit_id = sksu.u_id
                right join s_keeper_queue_filter as sk3 on sk3.q_id = sk1.queue_filter_id
                ) as c
                where old_qfid is null and new_qfid is not null"""  # or old_qfid is not null and new_qfid is null; """
                                            )
        serializer_class = Support_UnitPendingSerializer(queryset, many=True)
        return Response(serializer_class.data)


class Support_Unit_AbsentViewSet(viewsets.ModelViewSet):
    queryset = Support_Unit.objects.all()
    serializer_class = Support_UnitSerializer

    def get_queryset(self):
        return Support_Unit.objects.filter(is_absent=True)


class ComponentsDictViewSet(mixins.RetrieveModelMixin, mixins.ListModelMixin, mixins.CreateModelMixin,
                          mixins.UpdateModelMixin, mixins.DestroyModelMixin, viewsets.GenericViewSet):
    queryset = ComponentsDict.objects.all()
    serializer_class = ComponentsDictSerializer
    permission_classes = [IsAuthenticatedOrReadOnly, ]


class RanksDictViewSet(mixins.RetrieveModelMixin, mixins.ListModelMixin, mixins.CreateModelMixin,
                          mixins.UpdateModelMixin, mixins.DestroyModelMixin, viewsets.GenericViewSet):
    queryset = RanksDict.objects.all()
    serializer_class = RanksDictSerializer
    permission_classes = [IsAuthenticatedOrReadOnly, ]
