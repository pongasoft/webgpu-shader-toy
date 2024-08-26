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

#include "FragmentShader.h"

namespace shader_toy {

//------------------------------------------------------------------------
// FragmentShader::FragmentShader
//------------------------------------------------------------------------
FragmentShader::FragmentShader(Shader const &iShader) :
  fName{iShader.fName},
  fCode{iShader.fCode},
  fWindowSize{iShader.fWindowSize}
{
  if(iShader.fEditedCode)
  {
    auto &editor = edit();
    editor.SelectAll();
    editor.Paste(iShader.fEditedCode->c_str());
  }
}

//------------------------------------------------------------------------
// FragmentShader::getEditedCode
//------------------------------------------------------------------------
std::optional<std::string> shader_toy::FragmentShader::getEditedCode() const
{
  if(!fTextEditor)
    return std::nullopt;
  auto editedText = fTextEditor->GetText();
  if(editedText != getCode())
    return editedText;
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// FragmentShader::edit
//------------------------------------------------------------------------
TextEditor &FragmentShader::edit()
{
  if(!fTextEditor)
  {
    fTextEditor = TextEditor{};
    fTextEditor->SetLanguageDefinition(TextEditor::LanguageDefinitionId::None);
    fTextEditor->SetText(fCode);
    fTextEditor->SetShowWhitespacesEnabled(false);
  }
  return fTextEditor.value();
}

//------------------------------------------------------------------------
// FragmentShader::toggleRunning
//------------------------------------------------------------------------
void FragmentShader::toggleRunning(double iCurrentTime)
{
  if(fRunning)
    fInputs.time = static_cast<gpu::f32>(iCurrentTime - fStartTime);
  else
    fStartTime = iCurrentTime - fInputs.time;
  fRunning = ! fRunning;
}

//------------------------------------------------------------------------
// FragmentShader::getStatus
//------------------------------------------------------------------------
char const *FragmentShader::getStatus() const
{
  if(isCompiled())
  {
    if(isEnabled())
      return isRunning() ? "Running" : "Paused";
    else
      return "Disabled";
  }
  else
  {
    if(hasCompilationError())
      return "Error";
    else
      return "Compiling...";
  }
}

//------------------------------------------------------------------------
// FragmentShader::nextFrame
//------------------------------------------------------------------------
void FragmentShader::nextFrame(double iCurrentTime, int frameCount)
{
  fManualTime = true;

  fStartTime = iCurrentTime - fInputs.time;
  fInputs.frame += frameCount;
  fInputs.time = static_cast<gpu::f32>(iCurrentTime - fStartTime) + static_cast<float>(frameCount) / 60.0f;
}

//------------------------------------------------------------------------
// FragmentShader::previousFrame
//------------------------------------------------------------------------
void FragmentShader::previousFrame(double iCurrentTime, int frameCount)
{
  fManualTime = true;

  fStartTime = iCurrentTime - fInputs.time;
  fInputs.frame = std::max(0, fInputs.frame - frameCount);
  fInputs.time =
    std::max(0.0f, static_cast<gpu::f32>(iCurrentTime - fStartTime) - static_cast<float>(frameCount) / 60.0f);
}

//------------------------------------------------------------------------
// FragmentShader::stopManualTime
//------------------------------------------------------------------------
void FragmentShader::stopManualTime(double iCurrentTime)
{
  if(fManualTime)
  {
    if(fRunning)
      fStartTime = iCurrentTime - fInputs.time;
    fManualTime = false;
  }
}

//------------------------------------------------------------------------
// FragmentShader::updateCode
//------------------------------------------------------------------------
void FragmentShader::updateCode(std::string iCode)
{
  fCode = std::move(iCode);
  fState = State::NotCompiled{};
}

//------------------------------------------------------------------------
// FragmentShader::clone
//------------------------------------------------------------------------
std::unique_ptr<FragmentShader> FragmentShader::clone() const
{
  auto res = std::make_unique<FragmentShader>(*this);
  res->fState = State::NotCompiled{};
  return res;
}


}