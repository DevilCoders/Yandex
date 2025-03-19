from abc import ABC, abstractmethod
from enum import Enum
from typing import List, Union
from dataclasses import dataclass
from pathlib import Path
from pydub import AudioSegment

from .transcription import Transcription


class AudioProcessingType(Enum):
    Full = 'full'
    Stream = 'stream'


@dataclass
class RecognitionConfig:
    mode: AudioProcessingType = AudioProcessingType.Full
    language: str = None


class RecognitionModel(ABC):
    def __init__(self):
        pass

    @abstractmethod
    def transcribe(self, audio: AudioSegment, recognition_config: RecognitionConfig = None) -> List[Transcription]:
        """Transcribe audio. Returns transcriptions per channel"""
        pass

    def transcribe_file(self, audio_path: Union[str, Path], recognition_config: RecognitionConfig = None) -> List[Transcription]:
        """Transcribe audio from file. Returns transcriptions per channel"""
        audio = AudioSegment.from_file(str(audio_path))
        return self.transcribe(audio, recognition_config)
