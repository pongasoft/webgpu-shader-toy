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


namespace impl {

gpu::Renderable::Size value(json const &iObject, std::string const &iKey, gpu::Renderable::Size const &iDefaultValue)
{
  auto res = iDefaultValue;
  auto i = iObject.find(iKey);
  if(i != iObject.end() && i->is_object())
  {
    res.width = i->value("width", res.width);
    res.height = i->value("height", res.width);
  }
  return res;
}

}

//------------------------------------------------------------------------
// Preferences::loadState
//------------------------------------------------------------------------
State Preferences::loadState(std::string_view iKey, State const &iDefaultState)
{
  auto stateItem = fStorage->getItem(iKey);
  if(!stateItem)
    return iDefaultState;
  auto state = iDefaultState; // we initialize with the default state in case some values are missing
  auto data = json::parse(*stateItem);
  if(data.is_object())
  {
    state.fDarkStyle = data.value("fDarkStyle", state.fDarkStyle);
    state.fHiDPIAware = data.value("fHiDPIAware", state.fHiDPIAware);
    state.fAspectRatio = data.value("fAspectRatio", state.fAspectRatio);
    state.fMainWindowSize = impl::value(data, "fMainWindowSize", state.fMainWindowSize);
    state.fFragmentShaderWindowSize = impl::value(data, "fFragmentShaderWindowSize", state.fFragmentShaderWindowSize);
    if(data.find("fShaders") != data.end())
    {
      state.fShaders.clear();
      auto shaders = data.value("fShaders", json::array_t{});
      if(!shaders.empty())
      {
        for(auto &shader: shaders)
        {
          if(shader.is_object())
          {
            Shader s{};
            // TODO: this will throw exception if format is not right...
            s.fName = shader.at("fName");
            s.fCode = shader.at("fCode");
            state.fShaders.emplace_back(s);
          }
        }
      }
    }
    auto currentShader = data.value("fCurrentShader", "");
    if(!currentShader.empty())
      state.fCurrentShader = currentShader;
  }
  return state;
}

//------------------------------------------------------------------------
// Preferences::storeState
//------------------------------------------------------------------------
void Preferences::storeState(std::string_view iKey, State const &iState)
{
  fStorage->setItem(iKey, serialize(iState));
}

//------------------------------------------------------------------------
// Preferences::serialize
//------------------------------------------------------------------------
std::string Preferences::serialize(State const &iState)
{
  auto shaders = json::array();

  for(auto const &shader: iState.fShaders)
  {
    json s{
      {"fName", shader.fName},
      {"fCode", shader.fCode},
    };
    shaders.emplace_back(s);
  }

  json data{
    {"fFormatVersion", iState.fFormatVersion},
    {"fMainWindowSize", { {"width", iState.fMainWindowSize.width}, {"height", iState.fMainWindowSize.height} } },
    {"fFragmentShaderWindowSize", { {"width", iState.fFragmentShaderWindowSize.width}, {"height", iState.fFragmentShaderWindowSize.height} } },
    {"fDarkStyle", iState.fDarkStyle},
    {"fHiDPIAware", iState.fHiDPIAware},
    {"fAspectRatio", iState.fAspectRatio},
    {"fShaders", shaders}
  };


  if(iState.fCurrentShader)
    data["fCurrentShader"] = *iState.fCurrentShader;

  return data.dump();
}

}