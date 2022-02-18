#include "pch.h"
#include <iostream>
#include <string>
#include <filesystem>

#define DEFAULT_MESSAGE_TIME 480
#define SA2_SAVE_SIZE 24576

bool enabled;
bool forceReload;
bool deleteChao;
static char* newSavePath;
static char* slotSavePath;

int oldMenu;

bool hasInit;
bool hasChosenFile;
static std::string debugMessage;
static std::string extraDebugMessage;
int debugMessageTime;
int extraDebugMessageTime;

HelperFunctions HelperFunctionsGlobal;

void SetPathToRead()
{
	memcpy_s(&CurrentSavePath, 4, &newSavePath, 4);
}

void SetPathToWrite()
{
	memcpy_s(&CurrentSavePath, 4, &slotSavePath, 4);
}

void SetPathToNormal()
{
	int resourceGD1Pointer = 0x173D01C;
	WriteData(&CurrentSavePath, &resourceGD1Pointer, 4);
}

void ReloadSave();
void SetupDebugInfo(int scaleText, int colour);

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		HelperFunctionsGlobal = helperFunctions;
		hasInit = false;

		// Config start
		const IniFile* configFile = new IniFile(std::string(path) + "\\config.ini");

		enabled = configFile->getBool("QSRSettings", "Enabled", false);
		std::string premadeSave = configFile->getString("QSRSettings", "PremadeFile", "Clean");
		std::string saveFilePath = configFile->getString("QSRSettings", "SaveFilePath", "");
		int saveNum = configFile->getInt("QSRSettings", "SaveNum", 10);
		forceReload = configFile->getBool("QSRSettings", "ForceReload", false);
		deleteChao = configFile->getBool("QSRSettings", "DeleteChao", false);

		delete configFile;
		// Config End

		if (!enabled)
		{
			debugMessage = "Quick Save Reloader is disabled";
			debugMessageTime = DEFAULT_MESSAGE_TIME;
			return;
		}

		newSavePath = (char*) malloc(256); // Allocate string for the game to read our file
		slotSavePath = (char*) malloc(64); // Allocate string for writing savedata
		if (!newSavePath || !slotSavePath)
		{
			debugMessage = "Couldn't allocate memory for strings, somehow out of memory??";
			debugMessageTime = DEFAULT_MESSAGE_TIME;
			return;
		}
		
		sprintf_s(slotSavePath, 39, "./resource/gd_PC/SAVEDATA/SONIC2B__S%02d", saveNum);

		std::string loadedPath;
		if (premadeSave == "Custom")
		{
			if (saveFilePath.empty() || !std::filesystem::exists(saveFilePath))
			{
				debugMessage = "Custom Filepath provided doesn't exist";
				debugMessageTime = DEFAULT_MESSAGE_TIME;
				return;
			}
			else if (std::filesystem::file_size(saveFilePath) != SA2_SAVE_SIZE)
			{
				debugMessage = "Custom File provided isn't an SA2 Save File";
				debugMessageTime = DEFAULT_MESSAGE_TIME;
				return;
			}

			loadedPath = "./" + saveFilePath; // Set reload path to the custom provided one
			debugMessage = "Using \"" + saveFilePath + "\"";
		}
		else
		{
			loadedPath = ".\\" + std::string(path) + "\\premadeSaves\\" + premadeSave; // Set reload path to one of the ones inside premade saves
			debugMessage = "Using " + premadeSave;
		}
		strcpy_s(newSavePath, loadedPath.length() + 1, loadedPath.c_str());
		
		SetPathToRead();

		hasInit = true;
		hasChosenFile = false;
		extraDebugMessage = "Saving to File Number " + std::to_string(saveNum);
		extraDebugMessageTime = DEFAULT_MESSAGE_TIME;
		debugMessageTime = DEFAULT_MESSAGE_TIME;
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		if (!hasChosenFile && CurrentMenu == Menus_TitleScreen && oldMenu == Menus_FileSelect)
		{
			hasChosenFile = true;
			SetPathToWrite();
		}
		else if (hasChosenFile && CurrentMenu == Menus_Unknown_18 && oldMenu == Menus_Settings)
		{
			SetPathToNormal();
		}
		else if (hasChosenFile && CurrentMenu == Menus_Settings && oldMenu == Menus_Unknown_18)
		{
			SetPathToWrite();
		}

		if (!enabled && debugMessageTime == 0) return;

		if (!hasInit && debugMessageTime != 0)
		{
			
			SetupDebugInfo(18, 0xFFFF0000);
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 1), "WARNING:");
			SetupDebugInfo(18, 0xFF00FFAA);
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 2), debugMessage.c_str());

			debugMessageTime--;
		}
		else if (debugMessageTime != 0)
		{
			SetupDebugInfo(18, 0xFF00FFAA);
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 1), "SA2 Quick Save Reload");
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 3), debugMessage.c_str());

			debugMessageTime--;
		}

		if (extraDebugMessageTime != 0)
		{
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 4), extraDebugMessage.c_str());
			extraDebugMessageTime--;
		}

		oldMenu = CurrentMenu;
	}

	__declspec(dllexport) void __cdecl OnInput()
	{
		if (!hasInit || !hasChosenFile) return;

		if (forceReload && CurrentMenu == Menus_TitleScreen && oldMenu != Menus_TitleScreen)
		{
			ReloadSave();
		}
		else if (!forceReload && (CurrentMenu == Menus_TitleScreen || CurrentMenu == Menus_Main))
		{
			extraDebugMessage = "Press Y to reload save";
			extraDebugMessageTime = 1;

			PDS_PERIPHERAL* controller = ControllerPointers[0];

			if (controller && controller->press & Buttons_Y) // || keyboard handling maybe? Not possible cos sa2 doesn't map keyboard keys :)
			{
				ReloadSave();
			}
		}
		else
		{
			extraDebugMessageTime = 0;
		}
	}

	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer };
}

void ScaleDebugFontQSR(int scale)
{
	float FontScale;

	if ((float)HorizontalResolution / (float)VerticalResolution > 1.33f) 
		FontScale = floor((float)VerticalResolution / 480.0f);
	else 
		FontScale = floor((float)HorizontalResolution / 640.0f);

	HelperFunctionsGlobal.SetDebugFontSize(FontScale * scale);
}

void SetupDebugInfo(int scaleText, int colour)
{
	ScaleDebugFontQSR(scaleText);
	HelperFunctionsGlobal.SetDebugFontColor(colour);
}

void DeleteChaoSave()
{
	if (std::filesystem::exists("resource/gd_PC/SAVEDATA/SONIC2B__ALF"))
	{
		std::error_code exception;
		if (std::filesystem::remove("resource/gd_PC/SAVEDATA/SONIC2B__ALF", exception))
		{
			PrintDebug("Deleted Chao file");
			debugMessage = "Reloaded Savefile and Deleted Chao file";
		}
		else
		{
			PrintDebug("Couldn't delete Chao file");
			debugMessage = "Reloaded Savefile but couldn't delete Chao file";
		}
	}
	else
	{
		PrintDebug("Chao file doesn't exist");
		debugMessage = "Reloaded Savefile but Chao file doesn't exist";
	}
}

void ReloadSave()
{
	if (!std::filesystem::exists(newSavePath))
	{
		PrintDebug("Reload file doesn't exist anymore");
		debugMessage = "Reload file doesn't exist anymore";
		debugMessageTime = 100;
		return;
	}

	SetPathToRead();
	ProbablyLoadsSave(0);
	SetPathToWrite();

	PrintDebug("Reloaded Savefile\n");
	debugMessage = "Reloaded Savefile";

	if (deleteChao)
	{
		DeleteChaoSave();
	}

	debugMessageTime = 100;
}

