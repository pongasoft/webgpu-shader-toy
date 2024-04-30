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

#include "Preferences.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace shader_toy {


//------------------------------------------------------------------------
// Preferences::loadSize
//------------------------------------------------------------------------
gpu::Renderable::Size Preferences::loadSize(std::string_view iKey, gpu::Renderable::Size const &iDefaultSize)
{
  auto sizeItem = fStorage->getItem(iKey);
  if(!sizeItem)
    return iDefaultSize;
  else
  {
    auto data = json::parse(*sizeItem);
    return {data["width"].template get<int>(), data["height"].template get<int>()};
  }
}

//------------------------------------------------------------------------
// Preferences::storeSize
//------------------------------------------------------------------------
void Preferences::storeSize(std::string_view iKey, gpu::Renderable::Size iSize)
{
  json data{};
  data["width"] = iSize.width;
  data["height"] = iSize.height;
  fStorage->setItem(iKey, data.dump());
}

}