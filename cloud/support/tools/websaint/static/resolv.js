function sendrawdata() {
    document.getElementById('done').innerHTML = 'Done: ';
    document.getElementById('usestatus').disabled=true;
    document.getElementById('aname').disabled=true;
    document.getElementById('state').disabled=true;
    document.getElementById('resource').disabled=true;
    var radio = document.getElementsByName("typeID")
    var choice = "";
    var i;
    for (i = 0; i < radio.length; i++) {
        if (radio[i].checked) {
          choice = radio[i].value;
        }
    }
    function check(element) {
    if (document.getElementById(element).checked == true) {
        return true;
    }
    else {
         return false;
    }
    };
    var rawdata = document.getElementById('rawdata').value.split('\n');
    var total = parseInt(rawdata.length);
    var done = 0;
    for (var id in rawdata) {
        var postdata = {
        data: rawdata[id]
        };
        var json = JSON.stringify(postdata)
        var request = new XMLHttpRequest();
        var myHeaders = new Headers();
        request.open('POST', document.URL+'/rawdata', true);
        request.setRequestHeader("Access-Control-Allow-Origin", "*");
        request.setRequestHeader("X-Type", choice);
        request.setRequestHeader('Content-Type', 'application/json');
        request.responseType = 'json';
        request.send(json);
        request.onload = function () {
            if (this.status != 200) {
                console.log("ID: %s, status: %d",this.response,this.status);
            }
            for (var id in this.response) {
                var data = this.response[id];
                if (check('resource') == true) {
                         strid = id + " " + data['id'] ;
                    }
                else {
                    var strid = id;
                }
                switch (data['name']) {

                case 'individual' :
                    if (check('usestatus')) {
                    switch (data['paid']) {
                    case true :
                        if (check('state')) {
                            if (data['active']) {
                                 document.getElementById('fl-ap').innerHTML+=strid+'<br>';
                            }
                            else {
                                document.getElementById('fl-sp').innerHTML+=strid+'<br>';
                            }
                        }
                        else {
                                document.getElementById('fl-ap').innerHTML+=strid+'<br>';
                            }
                        break;
                    case false :
                        if (check('state')) {
                            if (data['active']) {
                                 document.getElementById('fl-at').innerHTML+=strid+'<br>';
                            }
                            else {
                                document.getElementById('fl-st').innerHTML+=strid+'<br>';
                            }
                        }
                        else {
                             document.getElementById('fl-at').innerHTML+=strid+'<br>';
                        }
                        break;
                    default :
                        document.getElementById('fl-st').innerHTML+=strid+'<br>';
                    }
                    }
                    else {
                         if (check('state')) {
                               if (data['active']) {
                                    document.getElementById('fl-ap').innerHTML+=strid+'<br>';
                               }
                               else {
                                   document.getElementById('fl-sp').innerHTML+=strid+'<br>';
                               }
                         }
                         else {
                                document.getElementById('fl-ap').innerHTML+=strid+'<br>';
                         }
                    }
                    break;
                case 'SERVICE':
                    document.getElementById('srv').innerHTML += strid+'<br>';
                    break;
                default:
                    if (check('aname') == true) {
                         strid += " " + data['name'] ;
                    }
                    if (check('usestatus')) {
                        switch (data['paid']) {
                        case true:
                            if (check('state')) {
                                if (data['active']) {
                                    document.getElementById('ul-ap').innerHTML+=strid+'<br>';
                                }
                                else {
                                    document.getElementById('ul-sp').innerHTML+=strid+'<br>';
                                }
                            }
                            else {
                                document.getElementById('ul-ap').innerHTML+=strid+'<br>';
                            }
                            break;
                        case false:
                            if (check('state')) {
                                if (data['active']) {
                                    document.getElementById('ul-at').innerHTML+=strid+'<br>';
                                }
                                else {
                                    document.getElementById('ul-st').innerHTML+=strid+'<br>';
                                }
                            }
                            else{
                                document.getElementById('ul-at').innerHTML+=strid+'<br>';
                            }
                            break;

                        }
                    }
                    else {
                         if (check('state')) {
                               if (data['active']) {
                                    document.getElementById('ul-ap').innerHTML+=strid+'<br>';
                               }
                               else {
                                   document.getElementById('ul-sp').innerHTML+=strid+'<br>';
                               }
                         }
                         else {
                                document.getElementById('ul-ap').innerHTML+=strid+'<br>';
                         }
                    }
                }
            }
        done+=1;
        document.getElementById('done').innerHTML = 'Done: '+parseInt(done/total*100)+'%';
        if (done == total) {
            document.getElementById('usestatus').disabled=false;
            document.getElementById('aname').disabled=false;
            document.getElementById('state').disabled=false;
        }
        }

        }

}

function changevision(id) {
    for (var i in document.getElementsByName(id)) {
        document.getElementsByName(id)[i].hidden = !document.getElementById(id).checked;
    }
}
