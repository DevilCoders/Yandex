from rest_framework.serializers import HyperlinkedModelSerializer, ModelSerializer, SlugRelatedField, CharField, \
    Serializer, ValidationError
from django.contrib.auth import authenticate

# import models
from .models import Queue_Filter, Support_Unit, ComponentsDict, RanksDict


class Support_UnitPendingSerializer(Serializer):
    login = CharField(max_length=255)
    new_qfid = CharField(max_length=255)
    q_name = CharField(max_length=255)


class Support_UnitSerializer(ModelSerializer):
    password = CharField(
        max_length=128,
        min_length=8,
        write_only=True
    )

    class Meta:
        model = Support_Unit
        # fields = '__all__'
        fields = ['login', 'rank', 'is_fivetwo', 'is_absent', 'is_duty', 'password']

        read_only_fields = ['token', 'login']

    def update(self, instance, validated_data):
        """ Updates User data """

        password = validated_data.pop('password', None)

        for key, value in validated_data.items():
            setattr(instance, key, value)

        if password is not None:
            instance.set_password(password)

        instance.save()

        return instance


class RegistrationSerializer(ModelSerializer):
    """
    serializer to register new users
    """
    password = CharField(
        max_length=128,
        min_length=8,
        write_only=True
    )
    token = CharField(max_length=255, read_only=True)

    class Meta:
        model = Support_Unit
        fields = ['login', 'password', 'token']

    def create(self, validated_data):
        return Support_Unit.objects.create_user(**validated_data)


class LoginSerializer(Serializer):
    login = CharField(max_length=255)  # , read_only=True)
    password = CharField(max_length=128, write_only=True)
    token = CharField(max_length=255, read_only=True)
    refresh_token = CharField(max_length=255, read_only=True)

    def validate(self, data):

        login = data.get('login', None)

        if login is None:
            raise ValidationError(
                'A login is obviously required to log in.'
            )

        password = data.get('password', None)

        if password is None:
            raise ValidationError(
                'A password is required to log in.'
            )

        user = authenticate(username=login, password=password)

        if user is None:
            raise ValidationError(
                'A user with this login and password was not found.'
            )

        return {
            'login': user.login,
            'token': user.token,
        }


class Queue_FilterSerializer(ModelSerializer):
    assignees = SlugRelatedField(many=True, queryset=Support_Unit.objects.all(), slug_field='login')

    class Meta:
        model = Queue_Filter
        fields = '__all__'


class ComponentsDictSerializer(HyperlinkedModelSerializer):
    class Meta:
        model = ComponentsDict
        fields = '__all__'


class RanksDictSerializer(HyperlinkedModelSerializer):
    class Meta:
        model = RanksDict
        fields = '__all__'
