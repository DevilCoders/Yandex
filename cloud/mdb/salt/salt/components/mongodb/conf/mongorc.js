
prompt = function() {
    var state = 'ERROR';
    var host = 'unknown';
    var rsStatus = db.getSiblingDB('admin').runCommand({replSetGetStatus: 1, forShell: 1});
    if (rsStatus.ok) {
      rsStatus.members.forEach(function(member) {
        if (member.self) {
          state = member.stateStr;
          host = member.name.split('.')[0]
        }
      });

      state = '' + rsStatus.set + ':' + host + ':' + state;
    } else {
      var info = rsStatus.info;
      if (info && info.length < 20) {
        state = info;  // "mongos", "configsvr"
      } else {
        throw _getErrorWithCode(rsStatus, "Failed:" + info);
      }
    }
    return state + '> ';
}
