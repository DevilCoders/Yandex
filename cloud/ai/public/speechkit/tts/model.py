import io
import uuid

import grpc
import pydub
from speechkit.common.utils import get_yandex_credentials

import yandex.cloud.ai.tts.v3.tts_pb2 as tts_proto
from yandex.cloud.ai.tts.v3.tts_service_pb2_grpc import SynthesizerStub


class SynthesisModel:

    def __init__(self,
                 endpoint: str,
                 use_ssl: bool,
                 branch_key_value: (str, str),
                 voice: str = None):

        opts = [("grpc.max_message_length", 128 * 1024 * 1024)]
        if use_ssl:
            cred = grpc.ssl_channel_credentials()
            self._channel = grpc.secure_channel(endpoint, cred, options=opts)
        else:
            self._channel = grpc.insecure_channel(endpoint, options=opts)

        while True:
            try:
                grpc.channel_ready_future(self._channel).result(timeout=10)
                break
            except grpc.FutureTimeoutError:
                print("Synthesizer service is temporarily unavailable")

        self.__voice = voice
        self.__role = None
        self.__speed = None
        self.__lufs = None
        self.__sample_rate = 44100
        self.__unsafe_mode = True

        self.__branch_key, self.__branch_value = branch_key_value

    def __audio_spec(self):
        # return tts_proto.AudioFormatOptions(
        #     raw_audio=tts_proto.RawAudio(
        #         audio_encoding=tts_proto.RawAudio.AudioEncoding.LINEAR16_PCM,
        #         sample_rate_hertz=self.__sample_rate
        #     )
        # )
        return tts_proto.AudioFormatOptions(
            container_audio=tts_proto.ContainerAudio(
                container_audio_type=tts_proto.ContainerAudio.ContainerAudioType.OGG_OPUS
            )
        )

    def synthesize(self, text):
        request_id = str(uuid.uuid4())

        headers = (('authorization', get_yandex_credentials()),
                   (self.__branch_key, self.__branch_value),
                   ('x-client-request-id', request_id))

        stub = SynthesizerStub(self._channel)

        request = tts_proto.UtteranceSynthesisRequest(
            output_audio_spec=self.__audio_spec(),
            unsafe_mode=self.__unsafe_mode
        )

        if isinstance(text, str):
            request.text = text
        # elif isinstance(text, TextTemplate):
        #     raise Exception("TextTemplate not supported yet")
        else:
            raise Exception(f"Class {type(text).__name__} is not supported input for synthesizer")

        if self.__voice is not None:
            hint = request.hints.add()
            hint.voice = self.__voice
        if self.__role is not None:
            hint = request.hints.add()
            hint.role = self.__role
        if self.__speed is not None:
            hint = request.hints.add()
            hint.speed = self.__speed
        if self.__lufs is not None:
            hint = request.hints.add()
            hint.volume = self.__lufs

        try:
            responses = stub.UtteranceSynthesis(request,
                                                metadata=headers)
            # return b"".join([response.audio_chunk.data for response in responses])
            # return self.__sample_rate, b"".join([response.audio_chunk.data for response in responses])
            # with open("debug.wav", "wb") as f:
            #     f.write(wav)
            with io.BytesIO(b"".join([response.audio_chunk.data for response in responses])) as wave:
                # return wave, self.__sample_rate
                result = pydub.AudioSegment.from_ogg(wave)
        except grpc.RpcError as e:
            raise e
        return result

    @property
    def speed(self) -> float:
        return self.__speed

    @speed.setter
    def speed(self, speed: float):
        assert speed is not None
        self.__speed = speed

    @property
    def role(self) -> float:
        return self.__role

    @role.setter
    def role(self, role: str):
        assert role is not None
        self.__role = role

    @property
    def volume(self) -> float:
        return self.__lufs

    @volume.setter
    def volume(self, volume: float):
        assert volume is not None
        self.__lufs = volume
