/*
 * Copyright (c) 2024 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#ifndef WGPU_SHADER_TOY_ERRORS_H
#define WGPU_SHADER_TOY_ERRORS_H

#include <stdexcept>
#include <string>
#include "fmt.h"

namespace pongasoft {

struct Exception : public std::logic_error {
  explicit Exception(std::string const &s) : std::logic_error(s.c_str()) {}
  explicit Exception(char const *s) : std::logic_error(s) {}

  [[ noreturn ]] static void throwException(char const *iMessage, char const *iFile, int iLine)
  {
    throw Exception(fmt::printf("%s:%d | %s", iFile, iLine, iMessage));
  }

  template<typename ... Args>
  [[ noreturn ]] static void throwException(char const *iMessage, char const *iFile, int iLine, const std::string& format, Args ... args)
  {
    throw Exception(fmt::printf(" %s:%d | %s | %s", iFile, iLine, iMessage, fmt::printf(format, std::forward<Args>(args)...)));
  }
};

#define WST_INTERNAL_ASSERT(test, ...) (test) == true ? (void)0 : pongasoft::Exception::throwException("CHECK FAILED: [" #test "]", __FILE__, __LINE__, ##__VA_ARGS__)


}

#endif //WGPU_SHADER_TOY_ERRORS_H
