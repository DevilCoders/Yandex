data:
    solomon:
        sa_id: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[service_account_id]') }}
        sa_private_key: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[private_key]') | yaml_dquote }}
        sa_key_id: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[key_id]') }}
