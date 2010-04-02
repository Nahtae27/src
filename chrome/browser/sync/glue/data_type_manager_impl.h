// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_IMPL_H__
#define CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_IMPL_H__

#include "chrome/browser/sync/glue/data_type_manager.h"

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_type.h"

class NotificationSource;
class NotificationDetails;

namespace browser_sync {

class DataTypeController;
class SyncBackendHost;

class DataTypeManagerImpl : public DataTypeManager,
                            public NotificationObserver {
 public:
  DataTypeManagerImpl(SyncBackendHost* backend,
                      const DataTypeController::TypeMap& controllers);
  virtual ~DataTypeManagerImpl();

  // DataTypeManager interface.
  virtual void Configure(const TypeSet& desired_types);

  virtual void Stop();

  virtual const DataTypeController::TypeMap& controllers() {
    return controllers_;
  };

  virtual State state() {
    return state_;
  }

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  // This task is used to handle the "download ready" callback from
  // the SyncBackendHost in response to our ConfigureDataTypes() call.
  // We don't use a raw callback here so we can handle the case where
  // this instance gets destroyed before the callback is invoked.
  class DownloadReadyTask : public CancelableTask {
   public:
    explicit DownloadReadyTask(DataTypeManagerImpl* dtm) : dtm_(dtm) {}
    virtual void Run() {
      if (dtm_)
        dtm_->DownloadReady();
    }
    virtual void Cancel() {
      dtm_ = NULL;
    }
   private:
    DataTypeManagerImpl* dtm_;
  };

  // Starts the next data type in the kStartOrder list, indicated by
  // the current_type_ member.  If there are no more data types to
  // start, the stashed start_callback_ is invoked.
  void StartNextType();

  // Callback passed to each data type controller on startup.
  void TypeStartCallback(DataTypeController::StartResult result);

  // Stops all data types.
  void FinishStop();
  void FinishStopAndNotify(ConfigureResult result);

  void Restart();
  void DownloadReady();
  void AddObserver(NotificationType type);
  void RemoveObserver(NotificationType type);
  void NotifyStart();
  void NotifyDone(ConfigureResult result);
  void ResumeSyncer();
  void PauseSyncer();

  SyncBackendHost* backend_;
  // Map of all data type controllers that are available for sync.
  // This list is determined at startup by various command line flags.
  const DataTypeController::TypeMap controllers_;
  State state_;
  DataTypeController* current_dtc_;
  CancelableTask* download_ready_task_;
  std::map<syncable::ModelType, int> start_order_;
  TypeSet last_requested_types_;
  std::vector<DataTypeController*> needs_start_;
  std::vector<DataTypeController*> needs_stop_;

  NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(DataTypeManagerImpl);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_IMPL_H__
