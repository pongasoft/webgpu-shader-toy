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
#include "Errors.h"

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
template<typename R>
class AddOrRemoveFragmentShaderAction : public MainWindowAction<R>
{
protected:
  void add()
  {
    fName = this->fMainWindow->addFragmentShaderAction(std::move(fFragmentShaderToAdd), fPosition)->getName();
  }

  int remove()
  {
    auto [oldShader, position] = this->fMainWindow->removeFragmentShaderAction(fName);
    fFragmentShaderToAdd = oldShader->clone();
    fPosition = position;
    return fPosition;
  }

protected:
  std::unique_ptr<FragmentShader> fFragmentShaderToAdd{};
  std::string fName{};
  int fPosition{-1};
};


//------------------------------------------------------------------------
// AddFragmentShaderAction
//------------------------------------------------------------------------
class AddFragmentShaderAction : public AddOrRemoveFragmentShaderAction<void>
{
public:
  void init(std::unique_ptr<FragmentShader> iFragmentShader, int iPosition)
  {
    fFragmentShaderToAdd = std::move(iFragmentShader);
    fPosition = iPosition;
    fDescription = fmt::printf("Add Shader %s", fFragmentShaderToAdd->getName());
  }

  result_t execute() override { return add(); }
  void undo() override { remove(); }
};

//------------------------------------------------------------------------
// MainWindow::addFragmentShaderAction
//------------------------------------------------------------------------
std::shared_ptr<FragmentShader> MainWindow::addFragmentShaderAction(std::unique_ptr<FragmentShader> iFragmentShader, int iPosition)
{
  WST_INTERNAL_ASSERT(iFragmentShader != nullptr);
  setCurrentFragmentShader(std::move(iFragmentShader));
  if(iPosition >= 0 && iPosition <= fFragmentShaders.size())
    fFragmentShaders.insert(fFragmentShaders.begin() + iPosition, fCurrentFragmentShader);
  else
    fFragmentShaders.emplace_back(fCurrentFragmentShader);
  fCurrentFragmentShaderNameRequest = fCurrentFragmentShader->getName();
  return fCurrentFragmentShader;
}

//------------------------------------------------------------------------
// MainWindow::onNewFragmentShader
//------------------------------------------------------------------------
void MainWindow::onNewFragmentShader(std::unique_ptr<FragmentShader> iFragmentShader)
{
  auto existingShader = findFragmentShaderByName(iFragmentShader->getName());
  if(existingShader)
  {
    fUndoManager.beginTx(fmt::printf("Replace Shader %s", iFragmentShader->getName()));
    auto position = removeFragmentShader(existingShader->getName());
    executeAction<AddFragmentShaderAction>(std::move(iFragmentShader), position);
    fUndoManager.commitTx();
  }
  else
  {
    executeAction<AddFragmentShaderAction>(std::move(iFragmentShader), -1);
  }
}

//------------------------------------------------------------------------
// RemoveFragmentShaderAction
//------------------------------------------------------------------------
class RemoveFragmentShaderAction : public AddOrRemoveFragmentShaderAction<int>
{
public:
  void init(std::string iName)
  {
    fName = std::move(iName);
    fDescription = fmt::printf("Remove Shader %s", fName);
  }

  result_t execute() override { return remove(); }
  void undo() override { add(); }
};

//------------------------------------------------------------------------
// MainWindow::removeFragmentShaderAction
//------------------------------------------------------------------------
std::pair<std::shared_ptr<FragmentShader>, int> MainWindow::removeFragmentShaderAction(std::string const &iName)
{
  auto shader = findFragmentShaderByName(iName);
  WST_INTERNAL_ASSERT(shader != nullptr);
  auto iter = std::find(fFragmentShaders.begin(), fFragmentShaders.end(), shader);
  auto position = std::distance(fFragmentShaders.begin(), iter);
  fFragmentShaders.erase(iter);
  if(!fFragmentShaders.empty())
  {
    if(!fCurrentFragmentShader || fCurrentFragmentShader->getName() != iName)
      setCurrentFragmentShader(fFragmentShaders[0]);
  }
  else
  {
    fCurrentFragmentShader = nullptr;
    setTitle("WebGPU Shader Toy");
    fFragmentShaderWindow->setTitle("WebGPU Shader Toy");
  }

  return {shader, position};
}

//------------------------------------------------------------------------
// MainWindow::removeFragmentShader
//------------------------------------------------------------------------
int MainWindow::removeFragmentShader(std::string const &iName)
{
  return executeAction<RemoveFragmentShaderAction>(iName);
}

//------------------------------------------------------------------------
// MainWindow::RenameFragmentShaderAction
//------------------------------------------------------------------------
class RenameFragmentShaderAction : public MainWindowAction<void>
{
public:
  void init(std::string iOldName, std::string iNewName)
  {
    fOldName = std::move(iOldName);
    fNewName = std::move(iNewName);
    fDescription = fmt::printf("Rename Shader %s -> %s", fOldName, fNewName);
  }

  result_t execute() override
  {
    fMainWindow->renameShaderAction(fOldName, fNewName);
  }

  void undo() override
  {
    fMainWindow->renameShaderAction(fNewName, fOldName);
  }

protected:
  std::string fOldName{};
  std::string fNewName{};
};

//------------------------------------------------------------------------
// MainWindow::renameShader
//------------------------------------------------------------------------
void MainWindow::renameShader(std::string const &iOldName, std::string const &iNewName)
{
  auto existingShader = findFragmentShaderByName(iNewName);
  if(existingShader)
  {
    fUndoManager.beginTx(fmt::printf("Rename Shader %s -> %s", iOldName, iNewName));
    removeFragmentShader(iNewName);
    executeAction<RenameFragmentShaderAction>(iOldName, iNewName);
    fUndoManager.commitTx();
  }
  else
  {
    executeAction<RenameFragmentShaderAction>(iOldName, iNewName);
  }
}

//------------------------------------------------------------------------
// MainWindow::renameShaderAction
//------------------------------------------------------------------------
void MainWindow::renameShaderAction(std::string const &iOldName, std::string const &iNewName)
{
  auto shader = findFragmentShaderByName(iOldName);
  WST_INTERNAL_ASSERT(shader != nullptr);
  shader->setName(iNewName);
}

}