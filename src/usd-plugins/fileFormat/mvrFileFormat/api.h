// Copyright 2023 NVIDIA CORPORATION
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OMNI_MVR_API_H_
#define OMNI_MVR_API_H_

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define MVR_API
#   define MVR_API_TEMPLATE_CLASS(...)
#   define MVR_API_TEMPLATE_STRUCT(...)
#   define EDF_LOCAL
#else
#   if defined(MVRFILEFORMAT_EXPORTS)
#       define MVR_API ARCH_EXPORT
#       define MVR_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define MVR_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define MVR_API ARCH_IMPORT
#       define MVR_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define MVR_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EDF_LOCAL ARCH_HIDDEN
#endif

#endif