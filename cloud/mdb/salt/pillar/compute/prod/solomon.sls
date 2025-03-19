data:
    solomon:
        sa_id: {{ salt.yav.get('ver-01enr16x8yxqg734z8x3094fxe[service_account_id]') }}
        sa_private_key: {{ salt.yav.get('ver-01enr16x8yxqg734z8x3094fxe[private_key]') | yaml_dquote }}
        sa_key_id: {{ salt.yav.get('ver-01enr16x8yxqg734z8x3094fxe[key_id]') }}
