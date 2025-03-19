from dataclasses import dataclass


@dataclass
class ScrappedData:
    key: str
    css_selector: str

