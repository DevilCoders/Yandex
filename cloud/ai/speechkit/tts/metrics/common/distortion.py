import os
import tempfile
import logging

import numpy as np
import sox
from scipy.io import wavfile

from audio import AudioSample


class DistortionType:
    REVERB = 'REVERB'
    SPEED = 'SPEED'
    OVERDRIVE = 'OVERDRIVE'
    TREMOLO = 'TREMOLO'
    PITCH = 'PITCH'
    DOWNSAMPLE = 'DOWNSAMPLE'
    REPEAT_RANDOM_PART = 'REPEAT_RANDOM_PART'


ALL_DISTORTIONS = [
    DistortionType.REVERB,
    DistortionType.SPEED,
    DistortionType.OVERDRIVE,
    DistortionType.TREMOLO,
    DistortionType.PITCH,
    DistortionType.DOWNSAMPLE,
    DistortionType.REPEAT_RANDOM_PART
]


class VoiceDistorter:
    @staticmethod
    def for_each(audios, processing_func):
        sox.logger.setLevel(logging.ERROR)
        transformer = sox.Transformer()
        processing_func(transformer)

        new_audios = []

        with tempfile.TemporaryDirectory() as tmp:
            src_audio_file = os.path.join(tmp, 'src.wav')
            dst_audio_file = os.path.join(tmp, 'dst.wav')

            for audio in audios:
                wavfile.write(src_audio_file, data=audio.data, rate=audio.sample_rate)
                transformer.build(src_audio_file, dst_audio_file)
                new_audio = AudioSample(file=dst_audio_file)
                new_audios.append(new_audio)

        return new_audios

    @staticmethod
    def reverb(
            audios,
            reverberance=50,
            high_freq_damping=50,
            room_scale=100,
            stereo_depth=100,
            pre_delay=0,
            wet_gain=0,
            wet_only=False
    ):
        return VoiceDistorter.for_each(
            audios,
            lambda tfm: tfm.reverb(
                reverberance,
                high_freq_damping,
                room_scale,
                stereo_depth,
                pre_delay,
                wet_gain,
                wet_only
            )
        )

    @staticmethod
    def phaser(
            audios,
            gain_in=0.8,
            gain_out=0.74,
            delay=3,
            decay=0.4,
            speed=0.5,
            modulation_shape='sinusoidal'
    ):
        return VoiceDistorter.for_each(
            audios,
            lambda tfm: tfm.phaser(
                gain_in,
                gain_out,
                delay,
                decay,
                speed,
                modulation_shape
            )
        )

    @staticmethod
    def speed(audios, ratio):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.speed(ratio))

    @staticmethod
    def echo(
            audios,
            gain_in=0.8,
            gain_out=0.9,
            n_echos=1,
            delays=[60],
            decays=[0.4]
    ):
        return VoiceDistorter.for_each(
            audios,
            lambda tfm: tfm.echo(
                gain_in,
                gain_out,
                n_echos,
                delays,
                decays
            )
        )

    @staticmethod
    def bandpass(audios, frequency, width_q=2.0, constant_skirt=False):
        return VoiceDistorter.for_each(
            audios,
            lambda tfm: tfm.bandpass(frequency, width_q, constant_skirt)
        )

    @staticmethod
    def overdrive(audios, gain_db=20.0, colour=20.0):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.overdrive(gain_db, colour))

    @staticmethod
    def tremolo(audios, speed=6.0, depth=40.0):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.tremolo(speed, depth))

    @staticmethod
    def norm(audios, db_level=-3.0):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.norm(db_level))

    @staticmethod
    def pitch(audios, n_semitones):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.pitch(n_semitones))

    @staticmethod
    def stretch(audios, factor, window=20):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.stretch(factor, window))

    @staticmethod
    def downsample(audios, factor=2):
        return VoiceDistorter.for_each(audios, lambda tfm: tfm.downsample(factor))

    @staticmethod
    def repeat_random_part(audios, frame_size, n_repetions):
        new_audios = []

        for audio in audios:
            distorted_data = np.copy(audio.data)

            repeat_offset = np.argmax(distorted_data)
            frame_size_in_samples = min(round(audio.sample_rate * frame_size), distorted_data.shape[0] - repeat_offset)
            assert frame_size_in_samples > 50
            # repeat_offset = np.random.randint(
            #     low=0.2 * distorted_data.shape[0],
            #     high=0.8 * distorted_data.shape[0] - frame_size_in_samples
            # )
            repeated_frame = distorted_data[repeat_offset: repeat_offset + frame_size_in_samples]

            distorted_data = np.hstack(
                tuple(
                    [distorted_data[: repeat_offset]] +
                    [
                        repeated_frame for i in range(n_repetions)
                    ] +
                    [distorted_data[repeat_offset + frame_size_in_samples:]]
                )
            )
            new_audio = AudioSample(data=distorted_data, sample_rate=audio.sample_rate)
            new_audios.append(new_audio)

        return new_audios

    @staticmethod
    def random_trim(audios, duration):
        new_audios = []

        for audio in audios:
            trimmed_data = np.copy(audio.data)

            duration_in_samples = round(audio.sample_rate * duration)
            trim_offset = np.random.randint(
                low=0.2 * trimmed_data.shape[0],
                high=0.8 * trimmed_data.shape[0] - duration_in_samples
            )

            trimmed_data = np.hstack(
                (
                    trimmed_data[: trim_offset],
                    np.ones(shape=(round(duration_in_samples * 0.1),)) * trimmed_data[trim_offset],
                    trimmed_data[trim_offset + duration_in_samples:]
                )
            )
            new_audio = AudioSample(data=trimmed_data, sample_rate=audio.sample_rate)
            new_audios.append(new_audio)

        return new_audios

    @staticmethod
    def apply(distortion_type, audios):
        if distortion_type == DistortionType.REVERB:
            return VoiceDistorter.reverb(audios, reverberance=50, wet_gain=10)

        if distortion_type == DistortionType.SPEED:
            return VoiceDistorter.speed(audios, ratio=1.3)

        if distortion_type == DistortionType.OVERDRIVE:
            result = VoiceDistorter.overdrive(audios, gain_db=20.0, colour=20)
            result = VoiceDistorter.norm(result, db_level=-10.0)
            return result

        if distortion_type == DistortionType.TREMOLO:
            return VoiceDistorter.tremolo(audios, speed=300, depth=100)

        if distortion_type == DistortionType.PITCH:
            return VoiceDistorter.pitch(audios, -4)

        if distortion_type == DistortionType.DOWNSAMPLE:
            return VoiceDistorter.downsample(audios, 6)

        if distortion_type == DistortionType.REPEAT_RANDOM_PART:
            return VoiceDistorter.repeat_random_part(audios, 0.3, 10)
