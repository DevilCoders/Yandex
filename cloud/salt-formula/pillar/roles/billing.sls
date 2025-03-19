{% set billing_version = '0.5.0-1782.190626' %}

yc-pinning:
  packages:
    yc-billing: {{ billing_version }}
    yc-billing-tests: {{ billing_version }}
    yc-billing-uploader: {{ billing_version }}
    yc-billing-engine: {{ billing_version }}
    yc-billing-solomon: {{ billing_version }}
    yc-metrics-collector: '0.1-2587.190625'
