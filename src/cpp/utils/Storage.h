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

#ifndef WGPU_SHADER_TOY_UTILS_STORAGE_H
#define WGPU_SHADER_TOY_UTILS_STORAGE_H

#include <string>

namespace pongasoft::utils {

class Storage
{
public:
  virtual ~Storage() = default;
  virtual std::optional<std::string> getItem(std::string_view iKey) = 0;
  virtual void setItem(std::string_view iKey, std::string_view iValue) = 0;

  inline std::string getItem(std::string_view iKey, std::string_view iDefaultValue)
  {
    auto item = getItem(iKey);
    return item ? *item : std::string(iDefaultValue);
  }
};

class JSStorage : public Storage
{
public:
  std::optional<std::string> getItem(std::string_view iKey) override;
  void setItem(std::string_view iKey, std::string_view iValue) override;
};

}


#endif //WGPU_SHADER_TOY_UTILS_STORAGE_H