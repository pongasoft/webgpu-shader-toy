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
  else
    return deserialize(*stateItem, iDefaultState);
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

  for(auto const &shader: iState.fShaders.fList)
  {
    json s{
      {"fName", shader.fName},
      {"fCode", shader.fCode},
      {"fWindowSize", { {"width", shader.fWindowSize.width}, {"height", shader.fWindowSize.height} } },
    };
    if(shader.fEditedCode)
      s["fEditedCode"] = shader.fEditedCode.value();
    shaders.emplace_back(s);
  }

  auto &settings = iState.fSettings;

  json data{
    {"fFormatVersion", iState.fFormatVersion},
    {"fType", "project"},
    {"fMainWindowSize", { {"width", settings.fMainWindowSize.width}, {"height", settings.fMainWindowSize.height} } },
    {"fFragmentShaderWindowSize", { {"width", settings.fFragmentShaderWindowSize.width}, {"height", settings.fFragmentShaderWindowSize.height} } },
    {"fDarkStyle", settings.fDarkStyle},
    {"fLayoutManual", settings.fLayoutManual},
    {"fLayoutSwapped", settings.fLayoutSwapped},
    {"fHiDPIAware", settings.fHiDPIAware},
    {"fFontSize", settings.fFontSize},
    {"fLineSpacing", settings.fLineSpacing},
    {"fCodeShowWhiteSpace", settings.fCodeShowWhiteSpace},
    {"fScreenshotMimeType", settings.fScreenshotMimeType},
    {"fScreenshotQualityPercent", settings.fScreenshotQualityPercent},
    {"fShaders", shaders}
  };


  if(iState.fShaders.fCurrent)
    data["fCurrentShader"] = *iState.fShaders.fCurrent;

  return data.dump();
}

//------------------------------------------------------------------------
// Preferences::deserialize
//------------------------------------------------------------------------
State Preferences::deserialize(std::string const &iState, State const &iDefaultState)
{
  auto state = iDefaultState; // we initialize with the default state in case some values are missing
  auto data = json::parse(iState);
  auto &settings = state.fSettings;
  if(data.is_object())
  {
    settings.fDarkStyle = data.value("fDarkStyle", settings.fDarkStyle);
    settings.fLayoutManual = data.value("fLayoutManual", settings.fLayoutManual);
    settings.fLayoutSwapped = data.value("fLayoutSwapped", settings.fLayoutSwapped);
    settings.fHiDPIAware = data.value("fHiDPIAware", settings.fHiDPIAware);
    settings.fFontSize = data.value("fFontSize", settings.fFontSize);
    settings.fLineSpacing = data.value("fLineSpacing", settings.fLineSpacing);
    settings.fCodeShowWhiteSpace = data.value("fCodeShowWhiteSpace", settings.fCodeShowWhiteSpace);
    settings.fScreenshotMimeType = data.value("fScreenshotMimeType", settings.fScreenshotMimeType);
    settings.fScreenshotQualityPercent = data.value("fScreenshotQualityPercent", settings.fScreenshotQualityPercent);
//    state.fAspectRatio = data.value("fAspectRatio", state.fAspectRatio);
    settings.fMainWindowSize = impl::value(data, "fMainWindowSize", settings.fMainWindowSize);
    settings.fFragmentShaderWindowSize = impl::value(data, "fFragmentShaderWindowSize", settings.fFragmentShaderWindowSize);
    if(data.find("fShaders") != data.end())
    {
      state.fShaders.fList.clear();
      auto shaders = data.value("fShaders", json::array_t{});
      if(!shaders.empty())
      {
        for(auto &shader: shaders)
        {
          if(shader.is_object())
          {
            try
            {
              Shader s{};
              s.fName = shader.at("fName");
              s.fCode = shader.at("fCode");
              if(shader.contains("fEditedCode"))
                s.fEditedCode = shader.at("fEditedCode");
              s.fWindowSize = impl::value(shader, "fWindowSize", state.fSettings.fFragmentShaderWindowSize);
              state.fShaders.fList.emplace_back(s);
            }
            catch(...)
            {
              printf("Warning: Invalid syntax detected [ignored]\n");
            }
          }
        }
      }
    }
    auto currentShader = data.value("fCurrentShader", "");
    if(!currentShader.empty())
      state.fShaders.fCurrent = currentShader;
  }
  return state;
}

}