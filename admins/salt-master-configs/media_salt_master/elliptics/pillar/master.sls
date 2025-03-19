arc2salt:
  api-token: {{ salt.yav.get('sec-01fz88k4yfrc269ykxqhep6jvw[arc-api-token]') | json }}
  arc-token: {{ salt.yav.get('sec-01fz88k4yfrc269ykxqhep6jvw[arc-token]') | json }}

master:
  user: robot-storage-duty
  private_key: {{ salt.yav.get('sec-01fz88k4yfrc269ykxqhep6jvw[salt_master_private]') | json }}
