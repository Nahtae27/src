// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USER_MANAGER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USER_MANAGER_IMPL_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "base/synchronization/lock.h"
#include "base/time.h"
#include "chrome/browser/chromeos/login/user.h"
#include "chrome/browser/chromeos/login/user_image_loader.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/profiles/profile_downloader_delegate.h"
#include "chrome/browser/sync/profile_sync_service_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class SkBitmap;
class FilePath;
class PrefService;
class ProfileDownloader;
class ProfileSyncService;

namespace chromeos {

class RemoveUserDelegate;

// Implementation of the UserManager.
class UserManagerImpl : public UserManager,
                        public ProfileDownloaderDelegate,
                        public ProfileSyncServiceObserver,
                        public content::NotificationObserver {
 public:
  // UserManager implementation:
  virtual ~UserManagerImpl();

  virtual const UserList& GetUsers() const OVERRIDE;
  virtual void UserLoggedIn(const std::string& email) OVERRIDE;
  virtual void DemoUserLoggedIn() OVERRIDE;
  virtual void GuestUserLoggedIn() OVERRIDE;
  virtual void EphemeralUserLoggedIn(const std::string& email) OVERRIDE;
  virtual void UserSelected(const std::string& email) OVERRIDE;
  virtual void SessionStarted() OVERRIDE;
  virtual void RemoveUser(const std::string& email,
                          RemoveUserDelegate* delegate) OVERRIDE;
  virtual void RemoveUserFromList(const std::string& email) OVERRIDE;
  virtual bool IsKnownUser(const std::string& email) const OVERRIDE;
  virtual const User* FindUser(const std::string& email) const OVERRIDE;
  virtual const User& GetLoggedInUser() const OVERRIDE;
  virtual User& GetLoggedInUser() OVERRIDE;
  virtual bool IsDisplayNameUnique(
      const std::string& display_name) const OVERRIDE;
  virtual void SaveUserOAuthStatus(
      const std::string& username,
      User::OAuthTokenStatus oauth_token_status) OVERRIDE;
  virtual void SaveUserDisplayEmail(const std::string& username,
                                    const std::string& display_email) OVERRIDE;
  virtual std::string GetUserDisplayEmail(
      const std::string& username) const OVERRIDE;
  virtual int GetLoggedInUserWallpaperIndex() OVERRIDE;
  virtual void GetLoggedInUserWallpaperProperties(User::WallpaperType* type,
                                                  int* index) OVERRIDE;
  virtual void SaveLoggedInUserWallpaperProperties(User::WallpaperType type,
                                                   int index) OVERRIDE;
  virtual void SaveUserDefaultImageIndex(const std::string& username,
                                         int image_index) OVERRIDE;
  virtual void SaveUserImage(const std::string& username,
                             const SkBitmap& image) OVERRIDE;
  virtual void SetLoggedInUserCustomWallpaperLayout(
      ash::WallpaperLayout layout) OVERRIDE;
  virtual void SaveUserImageFromFile(const std::string& username,
                                     const FilePath& path) OVERRIDE;
  virtual void SaveUserWallpaperFromFile(const std::string& username,
                                         const FilePath& path,
                                         ash::WallpaperLayout layout,
                                         WallpaperDelegate* delegate) OVERRIDE;
  virtual void SaveUserImageFromProfileImage(
      const std::string& username) OVERRIDE;
  virtual void DownloadProfileImage(const std::string& reason) OVERRIDE;
  virtual bool IsCurrentUserOwner() const OVERRIDE;
  virtual bool IsCurrentUserNew() const OVERRIDE;
  virtual bool IsCurrentUserEphemeral() const OVERRIDE;
  virtual bool IsUserLoggedIn() const OVERRIDE;
  virtual bool IsLoggedInAsDemoUser() const OVERRIDE;
  virtual bool IsLoggedInAsGuest() const OVERRIDE;
  virtual bool IsLoggedInAsStub() const OVERRIDE;
  virtual bool IsSessionStarted() const OVERRIDE;
  virtual void AddObserver(Observer* obs) OVERRIDE;
  virtual void RemoveObserver(Observer* obs) OVERRIDE;
  virtual void NotifyLocalStateChanged() OVERRIDE;
  virtual const SkBitmap& DownloadedProfileImage() const OVERRIDE;

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // ProfileSyncServiceObserver implementation.
  virtual void OnStateChanged() OVERRIDE;

 protected:
  UserManagerImpl();

  // Returns image filepath for the given user.
  FilePath GetImagePathForUser(const std::string& username);

  // Returns wallpaper/thumbnail filepath for the given user.
  FilePath GetWallpaperPathForUser(const std::string& username,
                                   bool is_thumbnail);

 private:
  friend class UserManagerImplWrapper;
  friend class UserManagerTest;

  // Loads |users_| from Local State if the list has not been loaded yet.
  // Subsequent calls have no effect. Must be called on the UI thread.
  void EnsureUsersLoaded();

  // Retrieves trusted device policies and removes users from the persistent
  // list if ephemeral users are enabled. Schedules a callback to itself if
  // trusted device policies are not yet available.
  void RetrieveTrustedDevicePolicies();

  // Returns true if trusted device policies have successfully been retrieved
  // and ephemeral users are enabled.
  bool AreEphemeralUsersEnabled() const;

  // Returns true if the user with the given email address is to be treated as
  // ephemeral.
  bool IsEphemeralUser(const std::string& email) const;

  // Returns the user with the given email address if found in the persistent
  // list. Returns |NULL| otherwise.
  const User* FindUserInList(const std::string& email) const;

  // Makes stub user the current logged-in user (for test paths).
  void StubUserLoggedIn();

  // Notifies on new user session.
  void NotifyOnLogin();

  // Reads user's oauth token status from local state preferences.
  User::OAuthTokenStatus LoadUserOAuthStatus(const std::string& username) const;

  void SetCurrentUserIsOwner(bool is_current_user_owner);

  // Sets one of the default images for the specified user and saves this
  // setting in local state.
  // Does not send LOGIN_USER_IMAGE_CHANGED notification.
  void SetInitialUserImage(const std::string& username);

  // Sets one of the default wallpapers for the specified user and saves this
  // settings in local state.
  void SetInitialUserWallpaper(const std::string& username);

  // Migrate the old wallpaper index to a new wallpaper structure.
  // The new wallpaper structure is:
  // { WallpaperType: RANDOM|CUSTOMIZED|DEFAULT,
  //   index: index of the default wallpapers }
  void MigrateWallpaperData();

  // Sets image for user |username| and sends LOGIN_USER_IMAGE_CHANGED
  // notification unless this is a new user and image is set for the first time.
  // If |image| is empty, sets a stub image for the user.
  void SetUserImage(const std::string& username,
                    int image_index,
                    const SkBitmap& image);

  void GetUserWallpaperProperties(const std::string& username,
                                 User::WallpaperType* type,
                                 int* index);
  void SaveUserWallpaperProperties(const std::string& username,
                                   User::WallpaperType type,
                                   int index);

  // Saves image to file, updates local state preferences to given image index
  // and sends LOGIN_USER_IMAGE_CHANGED notification.
  void SaveUserImageInternal(const std::string& username,
                             int image_index,
                             const SkBitmap& image);

  // Saves wallpaper to file, post task to generate thumbnail and updates local
  // state preferences.
  void SaveUserWallpaperInternal(const std::string& username,
                                 ash::WallpaperLayout layout,
                                 User::WallpaperType type,
                                 WallpaperDelegate* delegate,
                                 const SkBitmap& image);

  // Loads custom wallpaper thumbnail asynchronously.
  void LoadCustomWallpaperThumbnail(const std::string& email,
                                    ash::WallpaperLayout layout,
                                    const SkBitmap& wallpaper);

  // Caches the loaded wallpaper for the given user.
  void OnCustomWallpaperThumbnailLoaded(const std::string& email,
                                        const SkBitmap& wallpaper);

  // Updates the custom wallpaper thumbnail in wallpaper picker UI.
  void OnThumbnailUpdated(WallpaperDelegate* delegate);

  // Generates a 128x80 thumbnail and saves it to local file system.
  void GenerateUserWallpaperThumbnail(const std::string& username,
                                      User::WallpaperType type,
                                      WallpaperDelegate* delegate,
                                      const SkBitmap& wallpaper);

  // Saves image to file with specified path and sends LOGIN_USER_IMAGE_CHANGED
  // notification. Runs on FILE thread. Posts task for saving image info to
  // Local State on UI thread.
  void SaveImageToFile(const std::string& username,
                       const SkBitmap& image,
                       const FilePath& image_path,
                       int image_index);

  // Saves wallpaper to file with specified path. Runs on FILE thread. Posts
  // task for saving wallpaper info to Local State on UI thread.
  void SaveWallpaperToFile(const std::string& username,
                           const SkBitmap& wallpaper,
                           const FilePath& wallpaper_path,
                           ash::WallpaperLayout layout,
                           User::WallpaperType type);

  // Stores path to the image and its index in local state. Runs on UI thread.
  // If |is_async| is true, it has been posted from the FILE thread after
  // saving the image.
  void SaveImageToLocalState(const std::string& username,
                             const std::string& image_path,
                             int image_index,
                             bool is_async);

  // Stores layout and type preference in local state. Runs on UI thread.
  void SaveWallpaperToLocalState(const std::string& username,
                                 const std::string& wallpaper_path,
                                 ash::WallpaperLayout layout,
                                 User::WallpaperType type);

  // Saves |image| to the specified |image_path|. Runs on FILE thread.
  bool SaveBitmapToFile(const SkBitmap& image,
                        const FilePath& image_path);

  // Initializes |downloaded_profile_picture_| with the picture of the logged-in
  // user.
  void InitDownloadedProfileImage();

  // Deletes user's image file. Runs on FILE thread.
  void DeleteUserImage(const FilePath& image_path);

  // Updates current user ownership on UI thread.
  void UpdateOwnership(bool is_owner);

  // Checks current user's ownership on file thread.
  void CheckOwnership();

  // ProfileDownloaderDelegate implementation.
  virtual int GetDesiredImageSideLength() const OVERRIDE;
  virtual Profile* GetBrowserProfile() OVERRIDE;
  virtual std::string GetCachedPictureURL() const OVERRIDE;
  virtual void OnDownloadComplete(ProfileDownloader* downloader,
                                  bool success) OVERRIDE;

  // Creates a new User instance.
  User* CreateUser(const std::string& email) const;

  // Removes the user from the persistent list only. Also removes the user's
  // picture.
  void RemoveUserFromListInternal(const std::string& email);

  // Loads user image from its file.
  scoped_refptr<UserImageLoader> image_loader_;

  // List of all known users. User instances are owned by |this| and deleted
  // when users are removed by |RemoveUserFromListInternal|.
  mutable UserList users_;

  // Map of users' display names used to determine which users have unique
  // display names.
  mutable base::hash_map<std::string, size_t> display_name_count_;

  // The logged-in user. NULL until a user has logged in, then points to one
  // of the User instances in |users_|, the |guest_user_| instance or an
  // ephemeral user instance. In test paths without login points to the
  // |stub_user_| instance.
  User* logged_in_user_;

  // True if SessionStarted() has been called.
  bool session_started_;

  // Cached flag of whether currently logged-in user is owner or not.
  // May be accessed on different threads, requires locking.
  bool is_current_user_owner_;
  mutable base::Lock is_current_user_owner_lock_;

  // Cached flag of whether the currently logged-in user existed before this
  // login.
  bool is_current_user_new_;

  // Cached flag of whether the currently logged-in user is ephemeral. Storage
  // of persistent information is avoided for such users by not adding them to
  // the user list in local state, not downloading their custom user images and
  // mounting their cryptohomes using tmpfs.
  bool is_current_user_ephemeral_;

  User::WallpaperType current_user_wallpaper_type_;

  int current_user_wallpaper_index_;

  // Cached flag indicating whether ephemeral users are enabled. Defaults to
  // |false| if the value has not been read from trusted device policy yet.
  bool ephemeral_users_enabled_;

  // Cached name of device owner. Defaults to empty string if the value has not
  // been read from trusted device policy yet.
  std::string owner_email_;

  content::NotificationRegistrar registrar_;

  // Profile sync service which is observed to take actions after sync
  // errors appear. NOTE: there is no guarantee that it is the current sync
  // service, so do NOT use it outside |OnStateChanged| method.
  ProfileSyncService* observed_sync_service_;

  ObserverList<Observer> observer_list_;

  // Download user profile image on login to update it if it's changed.
  scoped_ptr<ProfileDownloader> profile_image_downloader_;

  // Arbitrary string passed to the last |DownloadProfileImage| call.
  std::string profile_image_download_reason_;

  // Time when the profile image download has started.
  base::Time profile_image_load_start_time_;

  // True if the last user image required async save operation (which may not
  // have been completed yet). This flag is used to avoid races when user image
  // is first set with |SaveUserImage| and then with |SaveUserImagePath|.
  bool last_image_set_async_;

  // Result of the last successful profile image download, if any.
  SkBitmap downloaded_profile_image_;

  // Data URL for |downloaded_profile_image_|.
  std::string downloaded_profile_image_data_url_;

  DISALLOW_COPY_AND_ASSIGN(UserManagerImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USER_MANAGER_IMPL_H_
