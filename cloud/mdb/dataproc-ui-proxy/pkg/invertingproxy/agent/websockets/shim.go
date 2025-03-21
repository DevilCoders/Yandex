/*
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// initially taken from https://github.com/google/inverting-proxy

package websockets

import (
	"io"
	"net/http"
	"strings"
	"text/template"
)

const (
	contentTypeHeader = "Content-Type"
	shimTemplate      = `
<!--START_WEBSOCKET_SHIM-->
<!--
    This file was served from behind a reverse proxy that does not support websockets.

    The following code snippet has been inserted by the proxy to replace websockets
    with HTTP requests that will work with this proxy.

    If this snippet insertion is causing issues, then contact the server administrator.
-->
<script>
(function() {
    if (typeof window.nativeWebSocket !== 'undefined') {
      // We have already replaced websockets
      return;
    }
    console.log('Replacing native websockets with a shim');
    window.nativeWebSocket = window.WebSocket;
    const  location = window.location;
    const shimUri = location.protocol + '//' + location.host + '/{{.ShimPath}}/';

    function shouldShimWebsockets(url) {
      var parsedURL = new URL(url);
      if (typeof parsedURL.host == 'undefined') {
        parsedURL.host = location.host;
      }
      return (parsedURL.host == location.host);
    }

    function WebSocketShim(url, protocols) {
      if (!shouldShimWebsockets(url)) {
        console.log("Not shimming websockets for " + parsedURL.host + " as it does not match the location of " + location.host);
        return new window.nativeWebSocket(url, protocols);
      }

      // We need to reference "this" within nested functions, so we alias it to "self"
      var self = this;

      this.readyState = WebSocketShim.CONNECTING;
      function openedHandler(msg) {
        self.readyState = WebSocketShim.OPEN;
        if (self.onopen) {
          self.onopen({ target: self });
        }
      }
      function receiveHandler(resp) {
        var msgs = JSON.parse(resp);
        if (self.onmessage) {
          msgs.forEach(function(msg) {
            if (Array.isArray(msg)) {
              msg = new Blob(msg);
            }
            self.onmessage({ target: self, data: msg });
          });
        }
      }
      function errorHandler() {
        if (self.onerror) {
          self.onerror({ target: self });
        }
      }
      self.xhr = function(action, msg, onsuccess, onexit) {
        var req = new XMLHttpRequest();
		req.timeout = 30000;
        req.onreadystatechange = function() {
          if (req.readyState === 4) {
            if (req.status === 200) {
              if (onsuccess) {
                onsuccess(req.responseText);
              }
            } else if (req.status === 408) {
              // request timed out on server; if it is /poll it will be restarted within onexit
            } else if (req.status === 0) {
              // client-side timeout went off; proceed to onexit()
            } else {
              // in case of any other error just treat connection as closed
              errorHandler();
              self.closedHandler();
			  return;
            }
            if (onexit) {
              onexit();
            }
          }
        };
        req.open("POST", shimUri + action, true);
        if (typeof msg !== 'string') {
          msg = JSON.stringify(msg);
        }
        req.send(msg);
      }

      self.closedHandler = function() {
        self.readyState = WebSocketShim.CLOSED;
        if (self.onclose) {
          self.onclose({ target: self });
        }
      }

      self.pendingMessages = [];
      self.needsConversion = function(msg) {
        if (typeof msg == 'string') {
          return false;
        }
        if (Array.isArray(msg)) {
          if (msg.length == 1) {
            if (typeof msg[0] == 'string') {
              return false;
            }
          }
        }
        return true;
      }
      self.convertMessagesAndPush = function(msgs) {
        for (var i = 0; i < msgs.length; i++) {
          if (self.needsConversion(msgs[i].msg)) {
            var blob = new Blob([msgs[i].msg]);
            var reader = new FileReader();
            reader.addEventListener("loadend", function() {
              msgs[i].msg = [reader.result];
              self.convertMessagesAndPush(msgs);
            });
            reader.readAsText(blob);
            return;
          }
        }
        self.xhr('data', msgs, null, function() {
          self.pushing = false;
          self.push();
        });
      }
      self.push = function() {
         if (self.pushing) {
           return;
         }
         if (self.pendingMessages.length == 0) {
           return;
         }
         self.pushing = true;
         var msgs = self.pendingMessages;
         self.pendingMessages = [];
         self.convertMessagesAndPush(msgs);
      }

      function poll() {
        if (self.readyState != WebSocketShim.OPEN) {
          return;
        }
        self.xhr('poll', {'id': self._sessionID}, receiveHandler, poll);
      }

      self.xhr('open', url, function(resp) {
        respJSON = JSON.parse(resp);
        self._sessionID = respJSON.id;
        openedHandler(respJSON.msg);
        poll();
      });
    }
    WebSocketShim.prototype = {
      binaryType: "blob",
      onopen: null,
      onclose: null,
      onmessage: null,
      onerror: null,

      send: function(data) {
        if (this.readyState != WebSocketShim.OPEN) {
          // if websocket connection is not open then browser (at least firefox) just ignores it
		  return;
        }
        this.pendingMessages.push({'id': this._sessionID, 'msg': data});
        this.push();
      },
      close: function() {
        if (this.readyState != WebSocketShim.OPEN) {
          return;
        }
        this.readyState = WebSocketShim.CLOSING;
        this.xhr('close', {'id': this._sessionID}, false, this.closedHandler);
      },
    };
    WebSocketShim.CONNECTING = 0;
    WebSocketShim.OPEN = 1;
    WebSocketShim.CLOSING = 2;
    WebSocketShim.CLOSED = 3;

    window.WebSocket = WebSocketShim;
})();
</script>
<!--END_WEBSOCKET_SHIM-->
`
)

var shimTmpl = template.Must(template.New("client-shim").Parse(shimTemplate))

type shimmedBody struct {
	reader io.Reader
	closer io.Closer
}

func (sb *shimmedBody) Read(p []byte) (n int, err error) {
	return sb.reader.Read(p)
}

func (sb *shimmedBody) Close() error {
	return sb.closer.Close()
}

func shimBody(resp *http.Response, shimCode string) error {
	if resp == nil || resp.Body == nil {
		// We have nothing to do on an empty response
		return nil
	}
	contentType := strings.ToLower(resp.Header.Get(contentTypeHeader))
	if !strings.Contains(contentType, "html") {
		// We only want to modify HTML responses
		return nil
	}
	wrapped := resp.Body

	// Read in the first kilobyte to see if the <head> tag exists in it
	buf := make([]byte, 1024)
	count, err := wrapped.Read(buf)
	if err != nil && err != io.EOF {
		return err
	}
	prefix := strings.Replace(string(buf[0:count]), "<head>", "<head>"+shimCode, 1)
	resp.Body = &shimmedBody{
		reader: io.MultiReader(strings.NewReader(prefix), wrapped),
		closer: wrapped,
	}
	resp.Header.Del("Content-Length")
	return nil
}
