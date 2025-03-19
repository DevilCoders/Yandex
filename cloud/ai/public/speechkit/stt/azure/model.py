import sys
import time
import json

from typing import List
from pydub import AudioSegment

from speechkit.common.utils import get_azure_credentials
from .. import AudioProcessingType, RecognitionConfig, RecognitionModel
from .. import Transcription, Word

try:
    import azure.cognitiveservices.speech as speechsdk
except ImportError:
    print(f'Failed to import Azure SpeechSDK\n'
          f'Install `azure-cognitiveservices-speech` to use AzureRecognizer', file=sys.stderr)


class AzureRecognizer(RecognitionModel):
    def __init__(self, **kwargs):
        super().__init__()

    def __transcribe(self, channel: int, pcm: bytes, sample_rate: int, sample_width: int, recognition_config: RecognitionConfig) -> Transcription:
        token, region = get_azure_credentials()
        speech_config = speechsdk.SpeechConfig(subscription=token, region=region)
        speech_config.speech_recognition_language = recognition_config.language
        speech_config.output_format = speechsdk.OutputFormat.Detailed

        stream = speechsdk.audio.PushAudioInputStream(
            stream_format=speechsdk.audio.AudioStreamFormat(
                samples_per_second=sample_rate,
                bits_per_sample=8 * sample_width,
                channels=1
            )
        )
        stream.write(pcm)
        stream.close()

        audio_config = speechsdk.audio.AudioConfig(stream=stream)
        speech_recognizer = speechsdk.SpeechRecognizer(speech_config=speech_config, audio_config=audio_config)

        recognition_events, done = [], False

        def on_recognized(e):
            nonlocal recognition_events
            recognition_events.append(e)

        def on_stop(_):
            speech_recognizer.stop_continuous_recognition()
            nonlocal done
            done = True

        speech_recognizer.recognized.connect(on_recognized)
        speech_recognizer.session_stopped.connect(on_stop)
        speech_recognizer.canceled.connect(on_stop)

        speech_recognizer.start_continuous_recognition()
        while not done:
            time.sleep(0.1)

        raw_recognitions, normalized_recognitions, words = [], [], []
        for event in recognition_events:
            r = json.loads(event.result.json)['NBest'][0]
            if 'Words' in r:
                raw_recognitions.append(r['Lexical'])
                normalized_recognitions.append(r['Display'])
                for word in r['Words']:
                    words.append(Word(word['Word'], word['Offset'] // 10000, (word['Offset'] + word['Duration']) // 10000))

        return Transcription(
            raw_text=' '.join(raw_recognitions),
            normalized_text=' '.join(normalized_recognitions),
            words=words,
            channel=str(channel)
        )

    def transcribe(self, audio: AudioSegment, recognition_config: RecognitionConfig = None) -> List[Transcription]:
        if recognition_config is None:
            recognition_config = RecognitionConfig()

        transcriptions = []
        for channel, content in enumerate(audio.split_to_mono()):
            transcriptions.append(
                self.__transcribe(channel, content.raw_data, audio.frame_rate, audio.sample_width, recognition_config))
        return transcriptions
