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
std::optional<std::string> FragmentShader::getEditedCode() const
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
void FragmentShader::toggleRunning()
{
  if(fClock.isRunning())
    fClock.pause();
  else
    fClock.resume();
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
// FragmentShader::updateInputsFromClock
//------------------------------------------------------------------------
void FragmentShader::updateInputsFromClock()
{
  fInputs.time = static_cast<gpu::f32>(fClock.getTime());
  fInputs.frame = fClock.getFrame();
}

//------------------------------------------------------------------------
// FragmentShader::setCompilationError
//------------------------------------------------------------------------
void FragmentShader::setCompilationError(State::CompiledInError const &iError)
{
  fState = iError;
  edit().AddErrorMarker(iError.fErrorLine, iError.fErrorColumn, iError.fErrorMessage);
}

//------------------------------------------------------------------------
// FragmentShader::resetTime
//------------------------------------------------------------------------
void FragmentShader::resetTime()
{
  fClock.reset();
  updateInputsFromClock();
}

//------------------------------------------------------------------------
// FragmentShader::tickTime
//------------------------------------------------------------------------
void FragmentShader::tickTime(double iTimeDelta)
{
  fClock.tickTime(iTimeDelta);
  updateInputsFromClock();
}

//------------------------------------------------------------------------
// FragmentShader::tickFrame
//------------------------------------------------------------------------
void FragmentShader::tickFrame(int iFrameCount)
{
  fClock.tickFrame(iFrameCount);
  updateInputsFromClock();
}

//------------------------------------------------------------------------
// FragmentShader::updateCode
//------------------------------------------------------------------------
void FragmentShader::updateCode(std::string iCode)
{
  fCode = std::move(iCode);
  if(!isCompilationPending())
  {
    edit().ClearErrorMarkers();
    fState = State::NotCompiled{};
  }
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