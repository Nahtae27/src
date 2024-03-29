// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A struct for managing data being dropped on a webview.  This represents a
// union of all the types of data that can be dropped in a platform neutral
// way.

#ifndef WEBKIT_GLUE_WEBDROPDATA_H_
#define WEBKIT_GLUE_WEBDROPDATA_H_

#include <map>
#include <string>
#include <vector>

#include "base/string16.h"
#include "googleurl/src/gurl.h"
#include "webkit/glue/webkit_glue_export.h"

struct IDataObject;

namespace WebKit {
class WebDragData;
}

struct WEBKIT_GLUE_EXPORT WebDropData {
  // The struct is used to represent a file in the drop data.
  struct WEBKIT_GLUE_EXPORT FileInfo {
    FileInfo();
    FileInfo(const string16& path, const string16& display_name);

    // The path of the file.
    string16 path;
    // The display name of the file. This field is optional.
    string16 display_name;
  };

  // Construct from a WebDragData object.
  explicit WebDropData(const WebKit::WebDragData&);

  WebDropData();

  ~WebDropData();

  // User is dragging a link into the webview.
  GURL url;
  string16 url_title;  // The title associated with |url|.

  // User is dragging a link out-of the webview.
  string16 download_metadata;

  // User is dropping one or more files on the webview.
  std::vector<FileInfo> filenames;

  // Isolated filesystem ID for the files being dragged on the webview.
  string16 filesystem_id;

  // User is dragging plain text into the webview.
  string16 plain_text;

  // User is dragging text/html into the webview (e.g., out of Firefox).
  // |html_base_url| is the URL that the html fragment is taken from (used to
  // resolve relative links).  It's ok for |html_base_url| to be empty.
  string16 text_html;
  GURL html_base_url;

  // User is dragging data from the webview (e.g., an image).
  string16 file_description_filename;
  std::string file_contents;

  std::map<string16, string16> custom_data;

  // Convert to a WebDragData object.
  WebKit::WebDragData ToDragData() const;

  // Helper method for converting Window's specific IDataObject to a WebDropData
  // object.  TODO(tc): Move this to the browser side since it's Windows
  // specific and no longer used in webkit.
  static void PopulateWebDropData(IDataObject* data_object,
                                  WebDropData* drop_data);
};

#endif  // WEBKIT_GLUE_WEBDROPDATA_H_
