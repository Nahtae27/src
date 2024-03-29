// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.getConfig(function(config) {
  var LOCAL_URL = 'local-iframe.html';
  var DATA_URL = 'data:text/plain,This frame should be displayed.';
  var REMOTE_URL = 'http://localhost:' + config.testServer.port
      '/files/extensions/platform_apps/iframes/remote-iframe.html';

  chrome.test.runTests([
    function localIframe() {
      var iframe = document.createElement('iframe');
      iframe.onload = chrome.test.callbackPass(function() {
        console.log('Local iframe loaded');
      });
      iframe.src = LOCAL_URL;
      document.body.appendChild(iframe);
    },

    function dataUrlIframe() {
      var iframe = document.createElement('iframe');
      iframe.onload = chrome.test.callbackPass(function() {
        console.log('data: URL iframe loaded');
      });
      iframe.src = DATA_URL;
      document.body.appendChild(iframe);
    },

    function filesystemUrlIframe() {
      var iframe = document.createElement('iframe');
      iframe.onload = chrome.test.callbackPass(function() {
        console.log('filesystem: URL iframe loaded');
      });

      webkitRequestFileSystem(
          window.TEMPORARY,
          1024,
          function(fs) {
            fs.root.getFile(
                'test.html',
                {create: true, exclusive: false},
                function(fileEntry) {
                    fileEntry.createWriter(function(fileWriter) {
                      fileWriter.onwriteend = function(e) {
                        var url = fileEntry.toURL();
                        chrome.test.assertEq(0, url.indexOf('filesystem:'));
                        iframe.src = url;
                        document.body.appendChild(iframe);
                      };

                      var blobBuilder = new WebKitBlobBuilder();
                      blobBuilder.append('This frame should be displayed');
                      fileWriter.write(blobBuilder.getBlob('text/html'));
                    });
                });
          });
    },

    function blobUrlIframe() {
      var blobBuilder = new WebKitBlobBuilder();
      blobBuilder.append('This frame should be displayed');

      var blobUrl = window.webkitURL.createObjectURL(
          blobBuilder.getBlob('text/html'));
      var iframe = document.createElement('iframe');
      iframe.onload = chrome.test.callbackPass(function() {
        console.log('blob: URL iframe loaded');
      });
      iframe.src = blobUrl;
      document.body.appendChild(iframe);
    },

    function FAILS_remoteIframe() {
      // TODO(mihaip): re-enable once remote iframe restrictions are
      // implemented.
      chrome.test.succeed();
      return;

      var iframe = document.createElement('iframe');
      iframe.onload = function() {
        chrome.test.notifyFail('Remote iframe should not have loaded');
      };
      iframe.src = REMOTE_URL;
      document.body.appendChild(iframe);

      // Load failure should happen synchronously, but wait a bit before
      // declaring success.
      setTimeout(chrome.test.succeed, 500);
    }
  ]);
});
