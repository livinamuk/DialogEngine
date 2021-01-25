
#include "DialogEngine.h"

std::vector<GameItem> DialogEngine::s_gameItems;
std::vector<GameFlag> DialogEngine::s_gameFlags;
std::vector<GameFloat> DialogEngine::s_gameFloats;
std::vector<DialogEntry> DialogEngine::s_dialogEntries;
std::vector<Response> DialogEngine::s_avaliableResponses;
std::vector<std::string> DialogEngine::s_pendingFusionActions; // these aren't finished yet
DialogEntry DialogEngine::s_currentDialog;
int DialogEngine::s_lastSelectedResponseIndex;

////////////////////////////////
//                            //
//   F I L E   L O A D I N G  // 
//                            //
//////////////////////////////// and saving...


void DialogEngine::SavePlayerFile(std::string filename)
{
	// Create and open a text file
	std::ofstream MyFile(filename);

	// Save flags
	for (GameFlag& gameFlag : s_gameFlags) {
		if (gameFlag.m_state)
			MyFile << gameFlag.m_name << "=true\n";
		else
			MyFile << gameFlag.m_name << "=false\n";
	}

	// Save floats
	for (GameFloat& gameFloat : s_gameFloats) {
		MyFile << gameFloat.m_name << "=" << (int)gameFloat.m_value << "\n";
	}

	// Save items
	for (GameItem& gameItem : s_gameItems) {
		MyFile << gameItem.m_name << "=give" << gameItem.m_quantity << "\n";
	}

	// Close the file
	MyFile.close();
}

void DialogEngine::LoadPlayerFile(std::string filename)
{
	std::ifstream file(filename);
	std::string line;
	while (getline(file, line))
	{
		int colonPos = line.find('=');
		std::string name = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1); // everything after the =

		// FIRST check if it is an item. Which is in the format: give=itemnameX and X is the quantity
		if (value.find("give") != std::string::npos)
		{
			// Check quantity
			int quantity = Util::StringToInt(value.substr(4));
			if (quantity == 0)
				quantity = 1;

			s_gameItems.push_back(GameItem(name, quantity));
			continue;
		}
		// Is it a boolean?
		if (value.find("true") != std::string::npos)
			SetGameFlag(name, true);
		else if (value.find("false") != std::string::npos)
			SetGameFlag(name, false);
		// Is it a float
		else {
			SetGameFloat(name, std::stof(value));
		}
	}
}


void DialogEngine::LoadDialogFile(std::string filename)
{
	std::ifstream file(filename);
	std::string line;
	DialogEntry tempDialogEntry = DialogEntry();

	while (getline(file, line))
	{
		// New dialog entry
		if (line[0] == '#')
		{
			// Add the last dialog entry to the vector, cause you're working on a new one now.
			if (tempDialogEntry.m_text != "NO TEXT") {
				s_dialogEntries.push_back(tempDialogEntry);
				// reset for reuse
				tempDialogEntry.m_ID = -1;
				tempDialogEntry.m_imageName = "NO IMAGE";
				tempDialogEntry.m_text = "NO TEXT";
				tempDialogEntry.m_responses.clear();
			}

			// ID
			std::string ID_string = line.substr(1, line.length() - 1);
			int ID = Util::StringToInt(ID_string);
			tempDialogEntry.m_ID = ID;
		}

		// Set image name
		if (line.substr(0, 3) == "IMG") {
			tempDialogEntry.m_imageName = line.substr(5, line.length() - 1);
		}

		// Set text
		if (line.substr(0, 4) == "TEXT") {
			tempDialogEntry.m_text = line.substr(6, line.length() - 1);
		}

		// Add response
		if (line.substr(0, 5) == "REPLY")
		{
			std::string wholeLine = line.substr(7, line.length() - 1);
			Response response;

			// Check for no special shit...
			if (wholeLine.find('[') == std::string::npos) {
				response.m_text = wholeLine;
			}
			// Well I guess it has some then...
			else
			{
				// First isolate the main text;
				response.m_text = wholeLine.substr(0, wholeLine.find('['));

				// Now isolate any special tags
				std::vector<std::string> tags;
				bool finished = false;
				while (!finished)
				{
					int begin = wholeLine.find('[');
					int end = wholeLine.find(']');
					std::string tag = wholeLine.substr(begin + 1, end - begin - 1);
					wholeLine = Util::RemoveFromBeginning(wholeLine, end + 1); // delete everything before the found closing tag
					tags.push_back(tag);

					if (wholeLine.find(']') == std::string::npos)
						finished = true;
				}

				// Now loop over those tags and "process" them
				for (std::string tag : tags)
				{
					// Fusion actions
					if (tag.substr(0, 14) == "FUSION_ACTION:")
					{
						// Isolate name
						int begin = tag.find('"');
						int end = tag.substr(begin + 1, tag.length()).find('"');
						response.m_fusionActions.push_back(tag.substr(begin + 1, end));
					}
					// Engine Actions
					else if (tag.substr(0, 7) == "ACTION:")
					{
						// Isolate name
						int begin = tag.find('"');
						int end = tag.substr(begin + 1, tag.length()).find('"');
						ResponseAction responseAction;
						responseAction.m_name = tag.substr(begin + 1, end);

						// Item action??
						if (tag.substr(end).find("_item") != std::string::npos)
						{
							std::string paramater = tag.substr(tag.find("_item") + 5);
							int quantity = Util::StringToFloat(paramater);

							// item action?
							if (tag.substr(end).find("give_item") != std::string::npos)
							{
								if (quantity == 0)
									quantity = 1;

								responseAction.m_actionType = ActionType::GIVE_ITEM;
								responseAction.m_modifierValue = quantity;
							}
							else if (tag.substr(end).find("take_item") != std::string::npos)
							{
								if (quantity == 0)
									quantity = 99999;

								responseAction.m_actionType = ActionType::TAKE_ITEM;
								responseAction.m_modifierValue = quantity;
							}
						}

						// bool action?
						else if (tag.substr(end).find("true") != std::string::npos)
							responseAction.m_actionType = ActionType::SET_BOOL_TRUE;

						else if (tag.substr(end).find("false") != std::string::npos)
							responseAction.m_actionType = ActionType::SET_BOOL_FALSE;

						// float action
						else if (tag.substr(end).find("-") != std::string::npos) {
							responseAction.m_modifierValue = Util::StringToFloat(tag.substr(tag.find("-") + 1));
							responseAction.m_actionType = ActionType::SUBRACT_FLOAT;
						}
						else if (tag.substr(end).find("+") != std::string::npos) {
							responseAction.m_modifierValue = Util::StringToFloat(tag.substr(tag.find("+") + 1));
							responseAction.m_actionType = ActionType::ADD_FLOAT;
						}
						else if (tag.substr(end).find("=") != std::string::npos) {
							responseAction.m_modifierValue = Util::StringToFloat(tag.substr(tag.find("=") + 1));
							responseAction.m_actionType = ActionType::SET_FLOAT;
						}
						response.m_responseActions.push_back(responseAction);
					}
					// Conditions
					else if (tag.substr(0, 10) == "CONDITION:")
					{
						// Isolate name
						int begin = tag.find('"');
						int end = tag.substr(begin + 1, tag.length()).find('"');
						std::string value = tag.substr(end); // this actually also includes the = or <= so be wary

						// Create condition
						ResponseCondition responseCondition;
						responseCondition.m_conditionName = tag.substr(begin + 1, end);

						// Is it a bool condition?
						if (value.find("false") != std::string::npos) {
							responseCondition.m_requiredConditionBoolState = false;
							responseCondition.m_ConditionType = ConditionType::BOOOL;
						}
						else if (value.find("true") != std::string::npos) {
							responseCondition.m_requiredConditionBoolState = true;
							responseCondition.m_ConditionType = ConditionType::BOOOL;
						}
						// Is it an item check
						else if (value.find("have_item") != std::string::npos) {
							responseCondition.m_ConditionType = ConditionType::HAVE_ITEM;
						}
						else if (value.find("no_item") != std::string::npos) {
							responseCondition.m_ConditionType = ConditionType::NO_ITEM;
						}
						// Must be a value comparison
						else {

							// isolate everything after the condition name, for example ">=0" or ">44"
							std::string rightSide = tag.substr(begin + responseCondition.m_conditionName.length() + 2);

							// determine the operator
							if (rightSide.find(">=") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_GREATER_OR_EQUAL;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find(">=") + 2));
							}
							else if (rightSide.find("<=") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_LESS_OR_EQUAL;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find("<=") + 2));
							}
							else if (rightSide.find("<") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_LESS;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find("<") + 1));
							}
							else if (rightSide.find(">") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_GREATER;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find(">") + 1));
							}
							else if (rightSide.find("==") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_EQUAL;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find("==") + 2));
							}
							else if (rightSide.find("!=") != std::string::npos) {
								responseCondition.m_ConditionType = ConditionType::FLOAT_NOT_EQUAL;
								responseCondition.m_conditionComparisonValue = Util::StringToFloat(rightSide.substr(rightSide.find("!=") + 2));
							}
						}
						response.m_responseConditions.push_back(responseCondition);
					}

					// Special color
					else if (tag == "SPECIAL_COLOR")
						response.m_isSpecialColor = true;

					// Goto
					else if (tag.find("GOTO") != std::string::npos)
					{
						// Then find the fucking ID
						response.m_gotoID = Util::StringToInt(tag.substr(tag.find(':') + 1));
					}
				}
			}
			tempDialogEntry.m_responses.push_back(response);
		}
	}

	// Well add the dialog entry you were last processing, cause they are only added when the next one is found remember.
	if (tempDialogEntry.m_text != "NO TEXT")
		s_dialogEntries.push_back(tempDialogEntry);

	// Close the file
	file.close();
}



///////////////////////////////
//                           //
//    M I S C    S T U F F   //
//                           //
///////////////////////////////


DialogEntry DialogEngine::GetDialogByID(int ID)
{
	// Find match
	for (DialogEntry& dialogEntry : s_dialogEntries) {
		if (dialogEntry.m_ID == ID)
			return dialogEntry;
	}
	// Otherwise return an empty new one
	return DialogEntry();
}

void DialogEngine::SetCurrentDialogByID(int ID)
{
	s_currentDialog = DialogEngine::GetDialogByID(ID);

	// Determine which responses should be shown
	UpdateAvaliableResponses();
}

std::string DialogEngine::GetCurrentDialogText()
{
	return s_currentDialog.m_text;
}

std::string DialogEngine::GetCurrentDialogImageName()
{
	return s_currentDialog.m_imageName;
}

bool DialogEngine::EvaluateConditionalComparison(std::string name, ConditionType conditionType, float comparisonValue)
{
	// If item or value is not found, assume 0, and perform check...
	int objectValue = 0;

	// Otherwise, get the value
	if (HasItem(name))
		objectValue = GetItemQuantity(name);
	if (GameFloatExists(name))
		objectValue = GetGameFloat(name);

	// Evaluate comparison
	if (conditionType == ConditionType::FLOAT_GREATER_OR_EQUAL)
		return (objectValue >= comparisonValue);

	else if (conditionType == ConditionType::FLOAT_GREATER)
		return (objectValue > comparisonValue);

	else if (conditionType == ConditionType::FLOAT_LESS_OR_EQUAL)
		return (objectValue <= comparisonValue);

	else if (conditionType == ConditionType::FLOAT_LESS)
		return (objectValue < comparisonValue);

	else if (conditionType == ConditionType::FLOAT_EQUAL)
		return (objectValue == comparisonValue);

	else if (conditionType == ConditionType::FLOAT_NOT_EQUAL)
		return (objectValue != comparisonValue);

	else return false;
}

void DialogEngine::UpdateAvaliableResponses()
{
	// Reset
	s_avaliableResponses.clear();

	// Get avaliable responses
	for (Response& response : s_currentDialog.m_responses)
	{
		// Check if all conditions are met
		for (ResponseCondition& responseCondition : response.m_responseConditions)
		{
			// Bool conditions
			if (responseCondition.m_ConditionType == ConditionType::BOOOL)
			{
				if (responseCondition.m_requiredConditionBoolState != DialogEngine::GetGameFlagState(responseCondition.m_conditionName))
					goto loop_exit;
			}
			// Could be an item check
			else if (responseCondition.m_ConditionType == ConditionType::HAVE_ITEM)
			{
				if (!DialogEngine::HasItem(responseCondition.m_conditionName))
					goto loop_exit;
			}
			else if (responseCondition.m_ConditionType == ConditionType::NO_ITEM)
			{
				if (DialogEngine::HasItem(responseCondition.m_conditionName))
					goto loop_exit;
			}
			// Must be a value or item comparison
			else
			{
				std::string name = responseCondition.m_conditionName;
				ConditionType type = responseCondition.m_ConditionType;
				float value = responseCondition.m_conditionComparisonValue;

				if (!DialogEngine::EvaluateConditionalComparison(name, type, value)) {
					//std::cout << "FAILED CHECK: " << response.m_text << "\n";
					goto loop_exit;
				}
			}
		}
		// It's good then
		s_avaliableResponses.push_back(response);

	loop_exit:;
	}
}

bool DialogEngine::IsDialogOver()
{
	if (s_currentDialog.m_ID == -1)
		return true;
	else
		return false;
}

std::string DialogEngine::GetResponseTextByIndex(int index)
{
	if (index < 0 || index >= s_avaliableResponses.size())
		return "out_of_range";

	return s_avaliableResponses[index].m_text;
}


void DialogEngine::SelectResponse(int index)
{
	s_lastSelectedResponseIndex = index;

	// Did something fuck up and you need to bail early?
	if (index < 0 || index >= s_avaliableResponses.size())
		return;

	// Otherwise begin
	Response selectedRespone = s_avaliableResponses[index];

	// Get any fusion actions
	for (std::string fusionAction : selectedRespone.m_fusionActions) {
		AddFusionAction(fusionAction);
		std::cout << "\nFUSION ACTION: " << fusionAction << "\n";
	}

	// Perform any other actions
	for (ResponseAction& responseAction : selectedRespone.m_responseActions)
	{
		// First you gotta find the GameValue based on the name.
		for (GameFloat& gameFloat : DialogEngine::s_gameFloats)
		{
			if (responseAction.m_name == gameFloat.m_name)
			{
				if (responseAction.m_actionType == ActionType::SUBRACT_FLOAT) {
					gameFloat.Subract(responseAction.m_modifierValue);
				}
				else if (responseAction.m_actionType == ActionType::ADD_FLOAT) {
					gameFloat.Add(responseAction.m_modifierValue);
				}
				else if (responseAction.m_actionType == ActionType::SET_FLOAT) {
					gameFloat.SetTo(responseAction.m_modifierValue);
				}
			}
		}
		// But what if it's a BOOL
		if (responseAction.m_actionType == ActionType::SET_BOOL_TRUE) {
			DialogEngine::SetGameFlag(responseAction.m_name, true);
		}
		else if (responseAction.m_actionType == ActionType::SET_BOOL_FALSE) {
			DialogEngine::SetGameFlag(responseAction.m_name, false);
		}

		// Ok but what if it's a GameItem kinda thing
		if (responseAction.m_actionType == ActionType::GIVE_ITEM) {
			DialogEngine::GiveItem(responseAction.m_name, responseAction.m_modifierValue);
		}
		if (responseAction.m_actionType == ActionType::TAKE_ITEM) {
			DialogEngine::TakeItem(responseAction.m_name, responseAction.m_modifierValue);
		}
	}

	// Switch to the new dialog
	SetCurrentDialogByID(selectedRespone.m_gotoID);
}

bool DialogEngine::IsResponseSpecialColored(int index)
{
	if (index < 0 || index >= s_avaliableResponses.size())
		return false;
	else
		return s_avaliableResponses[index].m_isSpecialColor;;
}

void DialogEngine::AddFusionAction(std::string name)
{
	s_pendingFusionActions.push_back(name);
}

bool DialogEngine::TriggerFusionAction(std::string name)
{
	for (int i = 0; i < s_pendingFusionActions.size(); i++)
	{
		if (s_pendingFusionActions[i] == name)
		{
			s_pendingFusionActions.erase(s_pendingFusionActions.begin() + i);
			return true;
		}
	}
	return false;
}

std::string DialogEngine::GetFusionActionNameByIndex(int index)
{
	if (index < 0 || index >= s_pendingFusionActions.size())
		return "out_of_range";

	return s_pendingFusionActions[index];
}

bool DialogEngine::CompareDialogImageNameToString(std::string query)
{
	return (GetCurrentDialogImageName() == query);
}

void DialogEngine::ClearAllData()
{
	s_gameItems.clear();
	s_gameFlags.clear();
	s_gameFloats.clear();
	s_dialogEntries.clear();
	s_avaliableResponses.clear();
	s_pendingFusionActions.clear();
	s_currentDialog.m_ID = -1;
}



///////////////////
//               //
//  F L O A T S  //
//               //
///////////////////


void DialogEngine::SetGameFloat(std::string name, float value)
{
	// check if float exists
	for (GameFloat& gameValue : s_gameFloats) {

		if (gameValue.m_name == name) {
			gameValue.SetTo(value);
			return;
		}
	}
	// Otherwise create it
	s_gameFloats.push_back(GameFloat(name, value));
}

void DialogEngine::SetGameFloatMin(std::string name, float value)
{
	for (GameFloat& gameValue : s_gameFloats)
		if (gameValue.m_name == name) {
			gameValue.SetMin(value);
			return;
		}
}

void DialogEngine::SetGameFloatMax(std::string name, float value)
{
	for (GameFloat& gameValue : s_gameFloats)
		if (gameValue.m_name == name) {
			gameValue.SetMax(value);
			return;
		}
}

void DialogEngine::AddToGameFloat(std::string name, float value)
{
	for (GameFloat& gameValue : s_gameFloats)
		if (gameValue.m_name == name) {
			gameValue.m_value += value;
			return;
		}
}

void DialogEngine::SubractFromGameFloat(std::string name, float value)
{
	for (GameFloat& gameValue : s_gameFloats)
		if (gameValue.m_name == name) {
			gameValue.m_value -= value;
			return;
		}
}


float DialogEngine::GetGameFloat(std::string flagName)
{
	// Look for the fucking flag
	for (GameFloat& gamevalue : s_gameFloats) {
		if (gamevalue.m_name == flagName) {
			return gamevalue.m_value;
		}
	}
	// returns negative nine nine nine nine nine if flag not found
	return -99999;
}


bool DialogEngine::GameFloatExists(std::string name)
{
	for (GameFloat& value : s_gameFloats)
		if (value.m_name == name)
			return true;
	return false;
}

std::string DialogEngine::GetGameFloatNameByIndex(int index)
{
	if (index < 0 || index >= s_gameFloats.size())
		return "out_of_range";

	return s_gameFloats[index].m_name;
}

/////////////////
//             //
//  F L A G S  //
//             //
/////////////////


void DialogEngine::SetGameFlag(std::string flagName, bool state)
{
	// check if flag exists
	for (GameFlag& gameFlag : s_gameFlags) {

		//  if (TCharCompare(gameFlag.m_name.c_str(), flagName)) {
		if (gameFlag.m_name == flagName) {
			gameFlag.m_state = state;
			return;
		}
	}
	// Otherwise create it
	s_gameFlags.push_back(GameFlag(flagName, state));
}

bool DialogEngine::GetGameFlagState(std::string flagName)
{
	// Look for the fucking flag
	for (GameFlag& gameFlag : s_gameFlags) {
		if (gameFlag.m_name == flagName) {
			return gameFlag.m_state;
		}
	}
	// returns false if flag not found
	return false;
}

void DialogEngine::ToggleGameFlag(std::string flagName)
{
	// Look for the fucking flag
	for (GameFlag& gameFlag : s_gameFlags) {
		if (gameFlag.m_name == flagName) {
			gameFlag.Toggle();
		}
	}
}

std::string DialogEngine::GetGameFlagNameByIndex(int index)
{
	if (index < 0 || index >= s_gameFlags.size())
		return "out_of_range";

	return s_gameFlags[index].m_name;
}



/////////////////
//             //
//  I T E M S  //
//             //
/////////////////


void DialogEngine::TakeItem(std::string name, int quantity)
{
	for (int i = 0; i < s_gameItems.size(); i++) {
		// Find the item
		if (s_gameItems[i].m_name == name) {
			// Subract from the item's quantity
			s_gameItems[i].m_quantity -= quantity;
			// If quanitity is 0 then remove it
			if (s_gameItems[i].m_quantity <= 0)
				s_gameItems.erase(s_gameItems.begin() + i);

			return;
		}
	}
}

void DialogEngine::GiveItem(std::string name, int quantity)
{
	for (int i = 0; i < s_gameItems.size(); i++) {
		// Find the item
		if (s_gameItems[i].m_name == name) {
			// Add to the item's quantity
			s_gameItems[i].m_quantity += quantity;
			return;
		}
	}
	// Oh you don't have it, well then create the fucking item.
	s_gameItems.push_back(GameItem(name, quantity));
}

int DialogEngine::GetItemQuantity(std::string name)
{
	for (GameItem& gameItem : s_gameItems)
		if (gameItem.m_name == name) {
			return gameItem.m_quantity;
		}
	return -1;
}

bool DialogEngine::HasItem(std::string name)
{
	for (GameItem& item : DialogEngine::s_gameItems)
		if (item.m_name == name)
			return true;
	return false;
}

std::string DialogEngine::GetGameItemNameByIndex(int index)
{
	if (index < 0 || index >= s_gameItems.size())
		return "out_of_range";

	return s_gameItems[index].m_name;
}
