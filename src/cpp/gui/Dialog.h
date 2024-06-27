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

namespace pongasoft::gui {

class Dialog
{
public:
  struct Button
  {
    using action_t = std::function<void()>;

    std::string fLabel{};
    action_t fAction{};
    bool fDefaultFocus{};
  };

  class Content
  {
  public:
    virtual ~Content() = default;
    virtual void render() = 0;
  };

  class LamdaContent : public Content
  {
  public:
    void render() override;

    std::function<void()> fLambda{};
    bool fCopyToClipboard{};
  };

public:
  explicit Dialog(std::string iTitle);
  void render();

  Dialog &preContentMessage(std::string iMessage) { fPreContentMessage = std::move(iMessage); return *this; }
  Dialog &postContentMessage(std::string iMessage) { fPostContentMessage = std::move(iMessage); return *this; }
  Dialog &text(std::string iText, bool iCopyToClipboard = false);
  Dialog &lambda(std::function<void()> iLambda, bool iCopyToClipboard = false);
  Dialog &button(std::string iLabel, Button::action_t iAction, bool iDefaultFocus = false);
  Dialog &buttonCancel(std::string iLabel = "Cancel", bool iDefaultFocus = false) { return button(std::move(iLabel), {}, iDefaultFocus); }
  Dialog &buttonOk(std::string iLabel = "Ok", bool iDefaultFocus = false) { return button(std::move(iLabel), {}, iDefaultFocus); }

  bool isOpen() const;

protected:
  float computeButtonWidth() const;

protected:
  std::string fTitle;
  std::optional<std::string> fPreContentMessage{};
  std::optional<std::string> fPostContentMessage{};
  std::vector<std::shared_ptr<Content>> fContent{};
  std::vector<Button> fButtons{};

private:
  std::string fDialogID;
};

}

#endif //WGPU_SHADER_TOY_GUI_DIALOG_H