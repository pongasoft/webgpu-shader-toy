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

#include "Storage.h"
#include <emscripten.h>
#include <vector>

//------------------------------------------------------------------------
// jsLocalStorageSetItem
//------------------------------------------------------------------------
EM_JS(void, jsLocalStorageSetItem, (char const *iKey, char const *iValue), {
  localStorage.setItem(UTF8ToString(iKey), UTF8ToString(iValue));
})

//------------------------------------------------------------------------
// jsLocalStorageGetItem
//------------------------------------------------------------------------
EM_JS(long, jsLocalStorageGetItem, (char const *iKey, char const *iValue, size_t iSize), {
  let value = localStorage.getItem(UTF8ToString(iKey));
  if(value === null)
    return -1;
  stringToUTF8(value, iValue, iSize);
  return value.length;
})

namespace pongasoft::utils {

//------------------------------------------------------------------------
// JSStorage::getItem
//------------------------------------------------------------------------
std::optional<std::string> JSStorage::getItem(std::string_view iKey)
{
  // ok because in single threaded environment (otherwise could use thread_local)
  static std::vector<char> kBuffer(1024);
  auto size = jsLocalStorageGetItem(iKey.data(), kBuffer.data(), kBuffer.size());
  if(size == -1)
    return std::nullopt;
  if(size <= kBuffer.size())
    return std::string(kBuffer.data());
  else
  {
    // make the buffer bigger and retry
    kBuffer.resize(size * 2);
    return getItem(iKey);
  }
}

//------------------------------------------------------------------------
// JSStorage::setItem
//------------------------------------------------------------------------
void JSStorage::setItem(std::string_view iKey, std::string_view iValue)
{
  jsLocalStorageSetItem(iKey.data(), iValue.data());
}

}