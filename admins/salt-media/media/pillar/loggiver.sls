schedule:
  loggiver:
    function: state.sls
    args:
      - loggiver
    kwargs:
      saltenv: stable
    when: 3:30am
