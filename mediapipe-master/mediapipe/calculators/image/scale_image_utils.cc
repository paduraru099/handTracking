// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mediapipe/calculators/image/scale_image_utils.h"

#include <math.h>

#include <string>

#include "absl/strings/str_split.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {
namespace scale_image {

namespace {
double ParseRational(const std::string& rational) {
  const std::vector<std::string> v = absl::StrSplit(rational, '/');
  const double numerator = std::strtod(v[0].c_str(), nullptr);
  const double denominator = std::strtod(v[1].c_str(), nullptr);
  return numerator / denominator;
}
}  // namespace

::mediapipe::Status FindCropDimensions(int input_width, int input_height,    //
                                       const std::string& min_aspect_ratio,  //
                                       const std::string& max_aspect_ratio,  //
                                       int* crop_width, int* crop_height,    //
                                       int* col_start, int* row_start) {
  CHECK(crop_width);
  CHECK(crop_height);
  CHECK(col_start);
  CHECK(row_start);

  double min_aspect_ratio_q = 0.0;
  double max_aspect_ratio_q = 0.0;
  if (!min_aspect_ratio.empty()) {
    min_aspect_ratio_q = ParseRational(min_aspect_ratio);
  }
  if (!max_aspect_ratio.empty()) {
    max_aspect_ratio_q = ParseRational(max_aspect_ratio);
  }

  *crop_width = input_width;
  *crop_height = input_height;
  *col_start = 0;
  *row_start = 0;

  // Determine the current aspect ratio.
  const double aspect_ratio =
      static_cast<double>(input_width) / static_cast<double>(input_height);

  if (!std::isinf(max_aspect_ratio_q) && !std::isinf(min_aspect_ratio_q)) {
    if (max_aspect_ratio_q > 0 && aspect_ratio > max_aspect_ratio_q) {
      // Determine the width based on the height multiplied by the max
      // aspect ratio.
      *crop_width = static_cast<int>(static_cast<double>(input_height) *
                                     max_aspect_ratio_q);
      *crop_width = (*crop_width / 2) * 2;
      // The col_start should be half the difference between the input width
      // and the output width.
      *col_start = (input_width - *crop_width) / 2;
    } else if (min_aspect_ratio_q > 0 && aspect_ratio < min_aspect_ratio_q) {
      // Determine the height based on the width divided by the min
      // aspect ratio.
      *crop_height = static_cast<int>(static_cast<double>(input_width) /
                                      min_aspect_ratio_q);
      *crop_height = (*crop_height / 2) * 2;
      *row_start = (input_height - *crop_height) / 2;
    }
  }

  CHECK_LE(*crop_width, input_width);
  CHECK_LE(*crop_height, input_height);
  return ::mediapipe::OkStatus();
}

::mediapipe::Status FindOutputDimensions(int input_width,                //
                                         int input_height,               //
                                         int target_width,               //
                                         int target_height,              //
                                         bool preserve_aspect_ratio,     //
                                         bool scale_to_multiple_of_two,  //
                                         int* output_width,
                                         int* output_height) {
  CHECK(output_width);
  CHECK(output_height);

  if (!preserve_aspect_ratio || (target_width <= 0 && target_height <= 0)) {
    if (target_width <= 0) {
      target_width = input_width;
    }
    if (target_height <= 0) {
      target_height = input_height;
    }
    if (scale_to_multiple_of_two) {
      *output_width = (target_width / 2) * 2;
      *output_height = (target_height / 2) * 2;
    } else {
      *output_width = target_width;
      *output_height = target_height;
    }
    return ::mediapipe::OkStatus();
  }

  if (target_width > 0) {
    // Try setting the height based on the width and the aspect ratio.
    int try_width = target_width;
    int try_height = static_cast<int>(static_cast<double>(target_width) /
                                      static_cast<double>(input_width) *
                                      static_cast<double>(input_height));
    try_width = (try_width / 2) * 2;
    try_height = (try_height / 2) * 2;

    if (target_height <= 0 || try_height <= target_height) {
      // The resulting height based on the target width and aspect ratio
      // was within the image, so use these dimensions.
      *output_width = try_width;
      *output_height = try_height;
      return ::mediapipe::OkStatus();
    }
  }

  if (target_height > 0) {
    // Try setting the width based on the height and the aspect ratio.
    int try_height = target_height;
    int try_width = static_cast<int>(static_cast<double>(target_height) /
                                     static_cast<double>(input_height) *
                                     static_cast<double>(input_width));
    try_width = (try_width / 2) * 2;
    try_height = (try_height / 2) * 2;

    if (target_width <= 0 || try_width <= target_width) {
      // The resulting width based on the target width and aspect ratio
      // was within the image, so use these dimensions.
      *output_width = try_width;
      *output_height = try_height;
      return ::mediapipe::OkStatus();
    }
  }
  RET_CHECK_FAIL()
      << "Unable to set output dimensions based on target dimensions.";
}

}  // namespace scale_image
}  // namespace mediapipe
