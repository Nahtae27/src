// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/file_writer_delegate.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/file_util_proxy.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "net/base/net_errors.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_writer.h"

namespace fileapi {

static const int kReadBufSize = 32768;

namespace {

base::PlatformFileError NetErrorToPlatformFileError(int error) {
// TODO(kinuko): Move this static method to more convenient place.
  switch (error) {
    case net::ERR_FILE_NO_SPACE:
      return base::PLATFORM_FILE_ERROR_NO_SPACE;
    case net::ERR_FILE_NOT_FOUND:
      return base::PLATFORM_FILE_ERROR_NOT_FOUND;
    case net::ERR_ACCESS_DENIED:
      return base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    default:
      return base::PLATFORM_FILE_ERROR_FAILED;
  }
}

}  // namespace

FileWriterDelegate::FileWriterDelegate(
    const FileSystemOperationInterface::WriteCallback& write_callback,
    scoped_ptr<FileWriter> file_writer)
    : write_callback_(write_callback),
      file_writer_(file_writer.Pass()),
      bytes_written_backlog_(0),
      bytes_written_(0),
      bytes_read_(0),
      io_buffer_(new net::IOBufferWithSize(kReadBufSize)),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {
}

FileWriterDelegate::~FileWriterDelegate() {
}

void FileWriterDelegate::Start(scoped_ptr<net::URLRequest> request) {
  request_ = request.Pass();
  request_->Start();
}

bool FileWriterDelegate::Cancel() {
  if (request_.get()) {
    // This halts any callbacks on this delegate.
    request_->set_delegate(NULL);
    request_->Cancel();
  }

  const int status = file_writer_->Cancel(
      base::Bind(&FileWriterDelegate::OnWriteCancelled,
                 weak_factory_.GetWeakPtr()));
  // Return true to finish immediately if we have no pending writes.
  // Otherwise we'll do the final cleanup in the Cancel callback.
  return (status != net::ERR_IO_PENDING);
}

void FileWriterDelegate::OnReceivedRedirect(net::URLRequest* request,
                                            const GURL& new_url,
                                            bool* defer_redirect) {
  NOTREACHED();
  OnError(base::PLATFORM_FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnAuthRequired(net::URLRequest* request,
                                        net::AuthChallengeInfo* auth_info) {
  NOTREACHED();
  OnError(base::PLATFORM_FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  NOTREACHED();
  OnError(base::PLATFORM_FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnSSLCertificateError(net::URLRequest* request,
                                               const net::SSLInfo& ssl_info,
                                               bool fatal) {
  NOTREACHED();
  OnError(base::PLATFORM_FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnResponseStarted(net::URLRequest* request) {
  DCHECK_EQ(request_.get(), request);
  if (!request->status().is_success() || request->GetResponseCode() != 200) {
    OnError(base::PLATFORM_FILE_ERROR_FAILED);
    return;
  }
  Read();
}

void FileWriterDelegate::OnReadCompleted(net::URLRequest* request,
                                         int bytes_read) {
  DCHECK_EQ(request_.get(), request);
  if (!request->status().is_success()) {
    OnError(base::PLATFORM_FILE_ERROR_FAILED);
    return;
  }
  OnDataReceived(bytes_read);
}

void FileWriterDelegate::Read() {
  bytes_written_ = 0;
  bytes_read_ = 0;
  if (request_->Read(io_buffer_.get(), io_buffer_->size(), &bytes_read_)) {
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&FileWriterDelegate::OnDataReceived,
                   weak_factory_.GetWeakPtr(), bytes_read_));
  } else if (!request_->status().is_io_pending()) {
    OnError(base::PLATFORM_FILE_ERROR_FAILED);
  }
}

void FileWriterDelegate::OnDataReceived(int bytes_read) {
  bytes_read_ = bytes_read;
  if (!bytes_read_) {  // We're done.
    OnProgress(0, true);
  } else {
    // This could easily be optimized to rotate between a pool of buffers, so
    // that we could read and write at the same time.  It's not yet clear that
    // it's necessary.
    cursor_ = new net::DrainableIOBuffer(io_buffer_, bytes_read_);
    Write();
  }
}

void FileWriterDelegate::Write() {
  int64 bytes_to_write = bytes_read_ - bytes_written_;
  int write_response =
      file_writer_->Write(cursor_,
                          static_cast<int>(bytes_to_write),
                          base::Bind(&FileWriterDelegate::OnDataWritten,
                                     weak_factory_.GetWeakPtr()));
  if (write_response > 0)
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&FileWriterDelegate::OnDataWritten,
                   weak_factory_.GetWeakPtr(), write_response));
  else if (net::ERR_IO_PENDING != write_response)
    OnError(NetErrorToPlatformFileError(write_response));
}

void FileWriterDelegate::OnDataWritten(int write_response) {
  if (write_response > 0) {
    OnProgress(write_response, false);
    cursor_->DidConsume(write_response);
    bytes_written_ += write_response;
    if (bytes_written_ == bytes_read_)
      Read();
    else
      Write();
  } else {
    OnError(NetErrorToPlatformFileError(write_response));
  }
}

void FileWriterDelegate::OnError(base::PlatformFileError error) {
  if (request_.get()) {
    request_->set_delegate(NULL);
    request_->Cancel();
  }

  write_callback_.Run(error, 0, true);
}

void FileWriterDelegate::OnProgress(int bytes_written, bool done) {
  DCHECK(bytes_written + bytes_written_backlog_ >= bytes_written_backlog_);
  static const int kMinProgressDelayMS = 200;
  base::Time currentTime = base::Time::Now();
  if (done || last_progress_event_time_.is_null() ||
      (currentTime - last_progress_event_time_).InMilliseconds() >
          kMinProgressDelayMS) {
    bytes_written += bytes_written_backlog_;
    last_progress_event_time_ = currentTime;
    bytes_written_backlog_ = 0;
    write_callback_.Run(
        base::PLATFORM_FILE_OK, bytes_written, done);
    return;
  }
  bytes_written_backlog_ += bytes_written;
}

void FileWriterDelegate::OnWriteCancelled(int status) {
  write_callback_.Run(base::PLATFORM_FILE_ERROR_ABORT, 0, true);
}

}  // namespace fileapi
