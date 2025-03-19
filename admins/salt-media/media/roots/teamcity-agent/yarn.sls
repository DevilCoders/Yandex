yarn-repo:
  pkgrepo.managed:
    - humanname: "Yarn repo"
    - name: deb https://dl.yarnpkg.com/debian/ stable main
    - file: /etc/apt/sources.list.d/yarn.list
    - keyid: 23E7166788B63E1E
    - keyserver: keyserver.ubuntu.com

  pkg.latest:
    - name: yarn
    - refresh: true
