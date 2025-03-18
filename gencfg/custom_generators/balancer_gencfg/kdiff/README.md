Next generation balancer config simplification and diff tool.

Developed under MINOTAUR-560 goal

Main idea:
- Unwrap all lua sections to json
- Remove some noise (host names, etc)
- Apply generalization procedure
    - Find common sections
    - Move them out to config common part

Written by kulikov@ in collaboration with mvel@
