/* Copyright 2021 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "jaxlib/rocm/hip_gpu_kernel_helpers.h"

#include <stdexcept>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

namespace jax {
namespace {
std::string ErrorString(hipError_t error) { return hipGetErrorString(error); }

std::string ErrorString(hipsparseStatus_t status) {
  // TODO(reza): check and see if we can use hipify
  switch (status) {
    case HIPSPARSE_STATUS_SUCCESS:
      return "hipSparse success.";
    case HIPSPARSE_STATUS_NOT_INITIALIZED:
      return "hipSparse has not been initialized.";
    case HIPSPARSE_STATUS_ALLOC_FAILED:
      return "hipSparse allocation failed.";
    case HIPSPARSE_STATUS_INVALID_VALUE:
      return "hipSparse invalid value error.";
    case HIPSPARSE_STATUS_ARCH_MISMATCH:
      return "hipSparse architecture mismatch error.";
    case HIPSPARSE_STATUS_MAPPING_ERROR:
      return "hipSpase mapping error.";
    case HIPSPARSE_STATUS_EXECUTION_FAILED:
      return "hipSparse execution failed.";
    case HIPSPARSE_STATUS_INTERNAL_ERROR:
      return "hipSparse internal error.";
    case HIPSPARSE_STATUS_MATRIX_TYPE_NOT_SUPPORTED:
      return "hipSparse matrix type not supported error.";
    case HIPSPARSE_STATUS_ZERO_PIVOT:
      return "hipSparse zero pivot error.";
    case HIPSPARSE_STATUS_NOT_SUPPORTED:
      return "hipSparse not supported error.";
    case HIPSPARSE_STATUS_INSUFFICIENT_RESOURCES:
      return "hipSparse insufficient reosourse error.";
    default:
      return absl::StrCat("Unknown hipSparse error: ", status, ".");
  }
}

std::string ErrorString(hipsolverStatus_t status) {
  switch (status) {
    case HIPSOLVER_STATUS_SUCCESS:
      return "hipSolver success.";
    case HIPSOLVER_STATUS_NOT_INITIALIZED:
      return "hipSolver has not been initialized.";
    case HIPSOLVER_STATUS_ALLOC_FAILED:
      return "hipSolver allocation failed.";
    case HIPSOLVER_STATUS_INVALID_VALUE:
      return "hipSolver invalid value error.";
    case HIPSOLVER_STATUS_MAPPING_ERROR:
      return "hipSolver mapping error.";
    case HIPSOLVER_STATUS_EXECUTION_FAILED:
      return "hipSolver execution failed.";
    case HIPSOLVER_STATUS_INTERNAL_ERROR:
      return "hipSolver internal error.";
    case HIPSOLVER_STATUS_NOT_SUPPORTED:
      return "hipSolver status not supported.";
    case HIPSOLVER_STATUS_ARCH_MISMATCH:
      return "hipSolver architecture mismatch error.";
    case HIPSOLVER_STATUS_HANDLE_IS_NULLPTR:
      return "hipSolver null pointer handle error.";
    case HIPSOLVER_STATUS_INVALID_ENUM:
      return "hipSolver unsupported enum status error.";
    default:
      return absl::StrCat("Unknown hipSolver error: ", status, ".");
  }
}

std::string ErrorString(hipblasStatus_t status) {
  switch (status) {
    case HIPBLAS_STATUS_SUCCESS:
      return "hipBlas success.";
    case HIPBLAS_STATUS_NOT_INITIALIZED:
      return "hipBlas has not been initialized.";
    case HIPBLAS_STATUS_ALLOC_FAILED:
      return "hipBlas resource allocation failed.";
    case HIPBLAS_STATUS_INVALID_VALUE:
      return "hipBlas invalid value error.";
    case HIPBLAS_STATUS_MAPPING_ERROR:
      return "hipBlas mapping error.";
    case HIPBLAS_STATUS_EXECUTION_FAILED:
      return "hipBlas execution failed.";
    case HIPBLAS_STATUS_INTERNAL_ERROR:
      return "hipBlas internal error.";
    case HIPBLAS_STATUS_NOT_SUPPORTED:
      return "hipBlas not supported error.";
    case HIPBLAS_STATUS_ARCH_MISMATCH:
      return "hipBlas architecture mismatch.";
    case HIPBLAS_STATUS_HANDLE_IS_NULLPTR:
      return "hipBlas null pointer handle error.";
    case HIPBLAS_STATUS_INVALID_ENUM:
      return "hipBlas unsupported enum status error.";
    default:
      return absl::StrCat("Unknown hipBlas error: ", status, ".");
  }
}

template <typename T>
std::string ErrorString(T status, const char* file, std::int64_t line,
                        const char* expr) {
  return absl::StrFormat("%s:%d: operation %s failed: %s", file, line, expr,
                         ErrorString(status));
}
}  // namespace

absl::Status AsStatus(hipError_t error, const char* file, std::int64_t line,
                      const char* expr) {
  if (error != hipSuccess)
    return absl::InternalError(ErrorString(error, file, line, expr));
  return absl::OkStatus();
}

absl::Status AsStatus(hipsolverStatus_t status, const char* file,
                      std::int64_t line, const char* expr) {
  if (status != HIPSOLVER_STATUS_SUCCESS)
    return absl::InternalError(ErrorString(status, file, line, expr));
  return absl::OkStatus();
}

absl::Status AsStatus(hipsparseStatus_t status, const char* file,
                      std::int64_t line, const char* expr) {
  if (status != HIPSPARSE_STATUS_SUCCESS)
    return absl::InternalError(ErrorString(status, file, line, expr));
  return absl::OkStatus();
}

absl::Status AsStatus(hipblasStatus_t status, const char* file,
                      std::int64_t line, const char* expr) {
  if (status != HIPBLAS_STATUS_SUCCESS)
    return absl::InternalError(ErrorString(status, file, line, expr));
  return absl::OkStatus();
}

absl::StatusOr<std::unique_ptr<void* []>>
MakeBatchPointers(hipStream_t stream, void* buffer, void* dev_ptrs, int batch,
                  int batch_elem_size) {
  char* ptr = static_cast<char*>(buffer);
  auto host_ptrs = absl::make_unique<void*[]>(batch);
  for (int i = 0; i < batch; ++i) {
    host_ptrs[i] = ptr;
    ptr += batch_elem_size;
  }
  JAX_RETURN_IF_ERROR(JAX_AS_STATUS(
      hipMemcpyAsync(dev_ptrs, host_ptrs.get(), sizeof(void*) * batch,
                     hipMemcpyHostToDevice, stream)));
  return std::move(host_ptrs);
}
}  // namespace jax
