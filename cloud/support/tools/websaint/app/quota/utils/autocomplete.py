#!/usr/bin/env python3
"""
Autocompletion for services.
Press [Tab] to complete the current service.
- The first Tab press fills in the common part of all completions
    and shows all the completions. (In the menu)
- Any following tab press cycles through all the possible completions.
"""

from prompt_toolkit.completion import WordCompleter
from app.quota.constants import SERVICES, SERVICE_ALIASES

services_completer = WordCompleter([service for service in (*SERVICES, *SERVICE_ALIASES)], ignore_case=True)
