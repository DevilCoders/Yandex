from datetime import datetime, timezone
import unittest

from cloud.ai.speechkit.tts.lib.data.model import (
    BucketAudioSource, SynthAudioSource, AudioAnnotation, SbSChoice, AudioSbSChoice,
)


class TestModel(unittest.TestCase):
    def test_serialization(self):
        yson = {
            'category': 'Category.ROBOT_VOICE',
            'damage_amount': 'DamageAmount.NO_DAMAGE',
            'type': 'bucket',
        }
        obj = BucketAudioSource(
            category='Category.ROBOT_VOICE',
            damage_amount='DamageAmount.NO_DAMAGE',
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, BucketAudioSource.from_yson(yson))

        yson = {
            'model_id': 'kuznetsov',
            'type': 'synth',
        }
        obj = SynthAudioSource(
            model_id='kuznetsov',
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, SynthAudioSource.from_yson(yson))

        yson = {
            'id': '0bc4366c-ad79-4a37-9303-922dbac5de15',
            'audio_id': '00057cdf-2d16-4339-b734-12dc47bd2568',
            'assignment_id': '0001806c60--60d0a94894c8742680dd1651',
            'question': 'noise',
            'answer': True,
            'audio_source': {
                'category': 'Category.DIFFERENT_SPEAKERS',
                'damage_amount': 'DamageAmount.MEDIUM_DAMAGE',
                'type': 'bucket',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            # below are datalens fields
            'audio_category': 'Category.DIFFERENT_SPEAKERS',
            'audio_damage_amount': 'DamageAmount.MEDIUM_DAMAGE',
            'audio_model_id': None,
            'audio_source_type': 'bucket',
            'received_at_ts': 946598400,
        }
        obj = AudioAnnotation(
            id='0bc4366c-ad79-4a37-9303-922dbac5de15',
            audio_id='00057cdf-2d16-4339-b734-12dc47bd2568',
            assignment_id='0001806c60--60d0a94894c8742680dd1651',
            question='noise',
            answer=True,
            audio_source=BucketAudioSource(
                category='Category.DIFFERENT_SPEAKERS',
                damage_amount='DamageAmount.MEDIUM_DAMAGE',
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, AudioAnnotation.from_yson(yson))

        yson = {
            'id': '0bc4366c-ad79-4a37-9303-922dbac5de15',
            'audio_id': '00057cdf-2d16-4339-b734-12dc47bd2568',
            'assignment_id': '0001806c60--60d0a94894c8742680dd1651',
            'question': 'noise',
            'answer': True,
            'audio_source': {
                'model_id': 'multispeaker',
                'type': 'synth',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            # below are datalens fields
            'audio_category': None,
            'audio_damage_amount': None,
            'audio_model_id': 'multispeaker',
            'audio_source_type': 'synth',
            'received_at_ts': 946598400,
        }
        obj = AudioAnnotation(
            id='0bc4366c-ad79-4a37-9303-922dbac5de15',
            audio_id='00057cdf-2d16-4339-b734-12dc47bd2568',
            assignment_id='0001806c60--60d0a94894c8742680dd1651',
            question='noise',
            answer=True,
            audio_source=SynthAudioSource(
                model_id='multispeaker',
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, AudioAnnotation.from_yson(yson))

        yson = {
            'id': '0bc4366c-ad79-4a37-9303-922dbac5de15',
            'assignment_id': '0001806c60--60d0a94894c8742680dd1651',
            'audio_left_id': '00057cdf-2d16-4339-b734-12dc47bd2568',
            'audio_right_id': '0082dfd5-8450-47ff-8a14-4b4186dd26cb',
            'choice': 'left',
            'audio_left_source': {
                'category': 'Category.DIFFERENT_SPEAKERS',
                'damage_amount': 'DamageAmount.MEDIUM_DAMAGE',
                'type': 'bucket',
            },
            'audio_right_source': {
                'model_id': 'multispeaker',
                'type': 'synth',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            # below are datalens fields
            'audio_left_category': 'Category.DIFFERENT_SPEAKERS',
            'audio_left_damage_amount': 'DamageAmount.MEDIUM_DAMAGE',
            'audio_left_model_id': None,
            'audio_left_source_type': 'bucket',
            'audio_right_category': None,
            'audio_right_damage_amount': None,
            'audio_right_model_id': 'multispeaker',
            'audio_right_source_type': 'synth',
            'received_at_ts': 946598400,
        }
        obj = AudioSbSChoice(
            id='0bc4366c-ad79-4a37-9303-922dbac5de15',
            assignment_id='0001806c60--60d0a94894c8742680dd1651',
            audio_left_id='00057cdf-2d16-4339-b734-12dc47bd2568',
            audio_right_id='0082dfd5-8450-47ff-8a14-4b4186dd26cb',
            choice=SbSChoice.LEFT,
            audio_left_source=BucketAudioSource(
                category='Category.DIFFERENT_SPEAKERS',
                damage_amount='DamageAmount.MEDIUM_DAMAGE',
            ),
            audio_right_source=SynthAudioSource(
                model_id='multispeaker',
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        self.maxDiff = 9999
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, AudioSbSChoice.from_yson(yson))
