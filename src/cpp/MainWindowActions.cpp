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

#include "MainWindow.h"
#include "fmt.h"

namespace shader_toy {

using namespace pongasoft::utils;

//------------------------------------------------------------------------
// MainWindow::executeAction
//------------------------------------------------------------------------
template<IsMainWindowAction T, class... Args>
typename T::result_t MainWindow::executeAction(Args &&... args)
{
  auto action = std::make_unique<T>();
  action->initTarget(this);
  action->init(std::forward<Args>(args)...);
  return fUndoManager.execute<typename T::result_t, typename T::action_t>(std::move(action));
}

//------------------------------------------------------------------------
// AddFragmentShaderAction
//------------------------------------------------------------------------
class AddFragmentShaderAction : public MainWindowAction<void>
{
public:
  void init(std::unique_ptr<FragmentShader> iFragmentShader, int iPosition)
  {
    fFragmentShader = std::move(iFragmentShader);
    fPosition = iPosition;
    fDescription = fmt::printf("Add Shader %s", fFragmentShader->getName());
  }

  result_t execute() override
  {
    fMainWindow->addFragmentShaderAction(fFragmentShader->clone(), fPosition);
  }

  void undo() override
  {
    auto [_, position] = fMainWindow->deleteFragmentShaderAction(fFragmentShader->getName());
    fPosition = position;
  }

private:
  std::unique_ptr<FragmentShader> fFragmentShader{};
  int fPosition{-1};
};

//------------------------------------------------------------------------
// MainWindow::addFragmentShaderAction
//------------------------------------------------------------------------
void MainWindow::addFragmentShaderAction(std::unique_ptr<FragmentShader> iFragmentShader, int iPosition)
{
  setCurrentFragmentShader(std::move(iFragmentShader));
  fFragmentShaders[fCurrentFragmentShader->getName()] = fCurrentFragmentShader;
  if(std::ranges::find(fFragmentShaderTabs, fCurrentFragmentShader->getName()) == fFragmentShaderTabs.end())
  {
    if(iPosition >= 0 && iPosition <= fFragmentShaderTabs.size())
      fFragmentShaderTabs.insert(fFragmentShaderTabs.begin() + iPosition, fCurrentFragmentShader->getName());
    else
      fFragmentShaderTabs.emplace_back(fCurrentFragmentShader->getName());
  }
  fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
}

//------------------------------------------------------------------------
// MainWindow::onNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::onNewFragmentShader(std::unique_ptr<FragmentShader> iFragmentShader)
{
  executeAction<AddFragmentShaderAction>(std::move(iFragmentShader), -1);
}

//------------------------------------------------------------------------
// DeleteFragmentShaderAction
//------------------------------------------------------------------------
class DeleteFragmentShaderAction : public MainWindowAction<std::shared_ptr<FragmentShader>>
{
public:
  void init(std::string iName)
  {
    fName = std::move(iName);
    fDescription = fmt::printf("Delete Shader %s", fName);
  }

  result_t execute() override
  {
    auto [oldShader, position] = fMainWindow->deleteFragmentShaderAction(fName);
    if(oldShader)
      fFragmentShader = oldShader->clone();
    fPosition = position;
    return oldShader;
  }

  void undo() override
  {
    if(fFragmentShader)
      fMainWindow->addFragmentShaderAction(fFragmentShader->clone(), fPosition);
  }

private:
  std::string fName{};
  std::unique_ptr<FragmentShader> fFragmentShader{};
  int fPosition{-1};
};

//------------------------------------------------------------------------
// MainWindow::deleteFragmentShaderAction
//------------------------------------------------------------------------
std::pair<std::shared_ptr<FragmentShader>, int> MainWindow::deleteFragmentShaderAction(std::string const &iName)
{
  auto position = -1;
  auto iterTab = std::find(fFragmentShaderTabs.begin(), fFragmentShaderTabs.end(), iName);
  if(iterTab != fFragmentShaderTabs.end())
  {
    position = std::distance(fFragmentShaderTabs.begin(), iterTab);
    fFragmentShaderTabs.erase(iterTab);
  }
  std::shared_ptr<FragmentShader> oldShader{};
  auto iter = fFragmentShaders.find(iName);
  if(iter != fFragmentShaders.end())
  {
    oldShader = iter->second;
    fFragmentShaders.erase(iter);
  }
  if(!fFragmentShaders.empty())
  {
    if(!fCurrentFragmentShader || fCurrentFragmentShader->getName() != iName)
    {
      setCurrentFragmentShader(fFragmentShaders[fFragmentShaderTabs[0]]);
    }
  }
  else
  {
    fCurrentFragmentShader = nullptr;
    setTitle("WebGPU Shader Toy");
    fFragmentShaderWindow->setTitle("WebGPU Shader Toy");
  }

  return {oldShader, position};
}

//------------------------------------------------------------------------
// MainWindow::deleteFragmentShader
//------------------------------------------------------------------------
std::shared_ptr<FragmentShader> MainWindow::deleteFragmentShader(std::string const &iName)
{
  return executeAction<DeleteFragmentShaderAction>(iName);
}

}