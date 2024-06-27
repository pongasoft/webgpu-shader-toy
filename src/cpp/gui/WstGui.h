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

#ifndef WGPU_SHADER_TOY_WSTGUI_H
#define WGPU_SHADER_TOY_WSTGUI_H

#include <imgui.h>
#include <string>

namespace pongasoft::gui::WstGui {

#define ReGui_Icon_Copy "C"

//------------------------------------------------------------------------
// WstGui::CenterNextWindow
//------------------------------------------------------------------------
inline void CenterNextWindow(ImGuiCond iFlags = ImGuiCond_Appearing)
{
  static ImVec2 mid = {0.5f, 0.5f};
  const auto center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, iFlags, mid);
}

//------------------------------------------------------------------------
// WstGui::MultiLineText
// Issues one ImGui::Text per line of text
//------------------------------------------------------------------------
void MultiLineText(std::string const &iText);

//------------------------------------------------------------------------
// WstGui::ShowTooltip
//------------------------------------------------------------------------
inline bool ShowTooltip()
{
  return ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
}

//------------------------------------------------------------------------
// ReGui::ToolTip
//------------------------------------------------------------------------
template<typename F>
void ToolTip(F &&iTooltipContent)
{
  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  iTooltipContent();
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}

//------------------------------------------------------------------------
// WstGui::CopyToClipboard
//------------------------------------------------------------------------
template<typename F>
void CopyToClipboard(F &&iContent)
{
  ImGui::PushID(&iContent);
  bool copy_to_clipboard = ImGui::Button(ReGui_Icon_Copy);
  if(WstGui::ShowTooltip())
  {
    WstGui::ToolTip([]{
      ImGui::TextUnformatted("Copy to clipboard");
    });
  }
  if(copy_to_clipboard)
  {
    ImGui::LogToClipboard();
  }
  iContent();
  if(copy_to_clipboard)
  {
    ImGui::LogFinish();
  }
  ImGui::PopID();
}
}

#endif //WGPU_SHADER_TOY_WSTGUI_H
