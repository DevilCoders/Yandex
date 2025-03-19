data:
  mdb-internal-api:
    config:
      logic:
        elasticsearch:
          versions:
          - version: 7.17.0
            name: '7.17'
            default: true
            deprecated: false
            updatable_to: []
          - version: 7.16.2
            name: '7.16'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
          - version: 7.15.2
            name: '7.15'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
          - version: 7.14.2
            name: '7.14'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
            - '7.15'
          - version: 7.13.4
            name: '7.13'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
            - '7.15'
            - '7.14'
          - version: 7.12.1
            name: '7.12'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
            - '7.15'
            - '7.14'
            - '7.13'
          - version: 7.11.2
            name: '7.11'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
            - '7.15'
            - '7.14'
            - '7.13'
            - '7.12'
          - version: 7.10.2
            name: '7.10'
            default: false
            deprecated: false
            updatable_to:
            - '7.17'
            - '7.16'
            - '7.15'
            - '7.14'
            - '7.13'
            - '7.12'
            - '7.11'
