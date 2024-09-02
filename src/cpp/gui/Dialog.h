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

#ifndef WGPU_SHADER_TOY_GUI_DIALOG_H
#define WGPU_SHADER_TOY_GUI_DIALOG_H

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include <type_traits>

namespace pongasoft::gui {

class IDialog
{
public:
  struct Button
  {
    using action_t = std::function<void()>;

    inline void execute() const { if(fAction) fAction(); }

    std::string fLabel{};
    action_t fAction{};
    bool fDefaultFocus{};
    bool fEnabled{true};
  };

  struct Content
  {
    using renderable_t = std::function<void()>;

    inline void render() const { if(fRenderable) fRenderable(); }

    renderable_t fRenderable{};
    bool fCopyToClipboard{};
  };

public:
  explicit IDialog(std::string iTitle);
  virtual ~IDialog() = default;
  void render();
  bool isOpen() const;
  void initKeyboardFocusHere();
  void dismiss() { fDialogIsNotDismissed = false; }

protected:
  float computeButtonWidth() const;
  void addContent(Content::renderable_t iRenderable, bool iCopyToClipboard);
  void addButton(std::string iLabel, Button::action_t iAction, bool iDefaultFocus);

protected:
  std::string fTitle;
  std::vector<Content> fContent{};
  std::vector<Button> fButtons{};
  bool fAllowDismissDialog{false};
  bool fDialogIsNotDismissed{true};
  bool fKeyboardFocusInitialized{false};

private:
  std::string fDialogID;
};

struct VoidState{};

template<typename T>
concept HasState = !std::is_same_v<T, VoidState>;

template<typename State>
class Dialog : public IDialog
{
public:
  explicit Dialog(std::string iTitle, State const &iState) : IDialog(std::move(iTitle)), fState{iState} {}

  Dialog &content(Content::renderable_t iRenderable, bool iCopyToClipboard = false) {
    addContent(std::move(iRenderable), iCopyToClipboard);
    return *this;
  }
  Dialog &content(std::function<void(Dialog &)> iContent, bool iCopyToClipboard = false) {
    return content([dialog = this, content = std::move(iContent)] { if(content) content(*dialog); }, iCopyToClipboard);
  }
  Dialog &button(std::string iLabel, Button::action_t iAction, bool iDefaultFocus = false) {
    addButton(std::move(iLabel), std::move(iAction), iDefaultFocus);
    return *this;
  }
  Dialog &button(std::string iLabel, std::function<void(Dialog &)> iAction, bool iDefaultFocus = false) {
    return button(std::move(iLabel), [dialog = this, action = std::move(iAction)] { if(action) action(*dialog); }, iDefaultFocus);
  }
  Dialog &buttonCancel(std::string iLabel = "Cancel", bool iDefaultFocus = false) { return button(std::move(iLabel), Button::action_t{}, iDefaultFocus); }
  Dialog &buttonOk(std::string iLabel = "Ok", bool iDefaultFocus = false) { return button(std::move(iLabel), Button::action_t{}, iDefaultFocus); }
  Dialog &allowDismissDialog() { fAllowDismissDialog = true; return *this; }

  State &state() noexcept requires HasState<State> { return fState; }
  Button &button(std::size_t iButton) { return fButtons[iButton]; }

private:
  State fState;
};

using DialogNoState = Dialog<VoidState>;

}

#endif //WGPU_SHADER_TOY_GUI_DIALOG_H