// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"

namespace {

// Implementation of Cronet_BufferCallback that calls free() to malloc() buffer.
class Cronet_BufferCallbackFree : public Cronet_BufferCallback {
 public:
  Cronet_BufferCallbackFree() = default;
  ~Cronet_BufferCallbackFree() override = default;

  // Singleton instance doesn't allow means to set app-specific context.
  void SetContext(Cronet_BufferCallbackContext context) override {}
  Cronet_BufferCallbackContext GetContext() override { return nullptr; }

  void OnDestroy(Cronet_BufferPtr buffer) override { free(buffer->GetData()); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Cronet_BufferCallbackFree);
};

base::LazyInstance<Cronet_BufferCallbackFree>::Leaky
    g_cronet_buffer_callback_free = LAZY_INSTANCE_INITIALIZER;

// Concrete implementation of abstract Cronet_Buffer interface.
class Cronet_BufferImpl : public Cronet_Buffer {
 public:
  Cronet_BufferImpl() = default;
  ~Cronet_BufferImpl() override;

  // Cronet_Buffer implementation
  void SetContext(Cronet_BufferContext context) override;
  Cronet_BufferContext GetContext() override;
  void InitWithDataAndCallback(RawDataPtr data,
                               uint64_t size,
                               Cronet_BufferCallbackPtr callback) override;
  void InitWithAlloc(uint64_t size) override;
  uint64_t GetSize() override;
  RawDataPtr GetData() override;

 private:
  RawDataPtr data_ = nullptr;
  uint64_t size_ = 0;
  Cronet_BufferCallbackPtr callback_ = nullptr;
  Cronet_BufferContext context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_BufferImpl);
};

Cronet_BufferImpl::~Cronet_BufferImpl() {
  if (callback_)
    callback_->OnDestroy(this);
}

void Cronet_BufferImpl::SetContext(Cronet_BufferContext context) {
  context_ = context;
}

Cronet_BufferContext Cronet_BufferImpl::GetContext() {
  return context_;
}

void Cronet_BufferImpl::InitWithDataAndCallback(
    RawDataPtr data,
    uint64_t size,
    Cronet_BufferCallbackPtr callback) {
  data_ = data;
  size_ = size;
  callback_ = callback;
}

void Cronet_BufferImpl::InitWithAlloc(uint64_t size) {
  data_ = malloc(size);
  if (!data_)
    return;
  size_ = size;
  callback_ = g_cronet_buffer_callback_free.Pointer();
}

uint64_t Cronet_BufferImpl::GetSize() {
  return size_;
}

RawDataPtr Cronet_BufferImpl::GetData() {
  return data_;
}

}  // namespace

CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_Create() {
  return new Cronet_BufferImpl();
}
