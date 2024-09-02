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

#include "Dialog.h"
#include "WstGui.h"
#include "../Errors.h"
#include <imgui.h>

namespace pongasoft::gui {

//------------------------------------------------------------------------
// IDialog::IDialog
//------------------------------------------------------------------------
IDialog::IDialog(std::string iTitle) : fTitle{std::move(iTitle)},
                                       fDialogID{fmt::printf("%s###Dialog", fTitle)}
{
  WST_INTERNAL_ASSERT(!fTitle.empty());
}

//------------------------------------------------------------------------
// IDialog::render
//------------------------------------------------------------------------
void IDialog::render()
{
  auto title = fDialogID.c_str();

  if(!ImGui::IsPopupOpen(title))
  {
    ImGui::OpenPopup(title);
    WstGui::CenterNextWindow();
  }

  auto needsSeparator = false;

  if(ImGui::BeginPopupModal(title, fAllowDismissDialog ? &fDialogIsNotDismissed : nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar))
  {
    for(auto &content: fContent)
    {
      if(needsSeparator)
        ImGui::Separator();
      content.render();
      needsSeparator = true;
    }

    if(needsSeparator)
    {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
    }

    auto needsSameLine = false;
    ImVec2 buttonSize{computeButtonWidth(), 0};

    for(auto &button: fButtons)
    {
      if(needsSameLine)
        ImGui::SameLine();

      ImGui::BeginDisabled(!button.fEnabled);
      {
        if(ImGui::Button(button.fLabel.c_str(), buttonSize))
        {
          button.execute();
          fDialogIsNotDismissed = false;
        }
        if(button.fDefaultFocus)
          ImGui::SetItemDefaultFocus();
        needsSameLine = true;
      }
      ImGui::EndDisabled();
    }

    if(!fDialogIsNotDismissed || fAllowDismissDialog && ImGui::IsKeyPressed(ImGuiKey_Escape))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

//------------------------------------------------------------------------
// IDialog::addContent
//------------------------------------------------------------------------
void IDialog::addContent(Content::renderable_t iRenderable, bool iCopyToClipboard)
{
  fContent.emplace_back(Content{.fRenderable = std::move(iRenderable), .fCopyToClipboard = iCopyToClipboard});
}

//------------------------------------------------------------------------
// IDialog::addButton
//------------------------------------------------------------------------
void IDialog::addButton(std::string iLabel, Button::action_t iAction, bool iDefaultFocus)
{
  WST_INTERNAL_ASSERT(!iLabel.empty());
  fButtons.emplace_back(Button{.fLabel = std::move(iLabel), .fAction = std::move(iAction), .fDefaultFocus = iDefaultFocus});
}

//------------------------------------------------------------------------
// IDialog::computeButtonWidth
//------------------------------------------------------------------------
float IDialog::computeButtonWidth() const
{
  float w{120.0f};

  for(auto &button: fButtons)
  {
    w = std::max(w, ImGui::CalcTextSize(button.fLabel.c_str()).x);
  }
  return w;
}

//------------------------------------------------------------------------
// IDialog::isOpen
//------------------------------------------------------------------------
bool IDialog::isOpen() const
{
  return ImGui::IsPopupOpen(fDialogID.c_str());
}

//------------------------------------------------------------------------
// IDialog::initKeyboardFocusHere
//------------------------------------------------------------------------
void IDialog::initKeyboardFocusHere()
{
  if(!fKeyboardFocusInitialized)
  {
    fKeyboardFocusInitialized = true;
    ImGui::SetKeyboardFocusHere();
  }
}

}