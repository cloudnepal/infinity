// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

module column_inverter;
import stl;
import column_vector;
import internal_types;

namespace infinity {

RefCount::RefCount() : lock_(), cv_(), ref_count_(0u) {}

RefCount::~RefCount() {}

void RefCount::Retain() noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    ++ref_count_;
}

void RefCount::Release() noexcept {
    std::lock_guard<std::mutex> guard(lock_);
    --ref_count_;
    if (ref_count_ == 0u) {
        cv_.notify_all();
    }
}

void RefCount::WaitForZeroRefCount() {
    std::unique_lock<std::mutex> guard(lock_);
    cv_.wait(guard, [this] { return (ref_count_ == 0u); });
}

bool RefCount::ZeroRefCount() {
    std::unique_lock<std::mutex> guard(lock_);
    return (ref_count_ == 0u);
}

} // namespace infinity