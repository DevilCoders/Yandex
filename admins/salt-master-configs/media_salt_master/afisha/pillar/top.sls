base:
  'afisha-salt*':
    - master
    - master-salt
  'salt*':
    - master
    - master-salt
  'c:afisha-dev':
    - match: grain
    - afisha-dev
  'c:afisha-load':
    - match: grain
    - afisha-load
  'c:afisha-prestable':
    - match: grain
    - afisha-prestable
  'c:afisha-qa':
    - match: grain
    - afisha-qa
