#include "pch.h"
#include <iostream>
#include <string>
#include <filesystem>

#define DEFAULT_MESSAGE_TIME 480

bool enabled;
static char* newSavePath;
static char* slotSavePath;

int oldMenu;

bool hasInit;
std::string debugMessage;
std::string extraDebugMessage;
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

		delete configFile;
		// Config End

		if (!enabled)
		{
			debugMessage = "Quick Save Reloader is disabled.";
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
				debugMessage = "Custom Filepath provided doesn't exist.";
				debugMessageTime = DEFAULT_MESSAGE_TIME;
				return;
			}
			else if (std::filesystem::file_size(saveFilePath) != 24576)
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
		extraDebugMessage = "Saving to File Number " + std::to_string(saveNum);
		extraDebugMessageTime = DEFAULT_MESSAGE_TIME;
		debugMessageTime = DEFAULT_MESSAGE_TIME;
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		if (!enabled && debugMessageTime == 0) return;

		if (CurrentMenu == Menus_TitleScreen && oldMenu != Menus_TitleScreen && hasInit)
		{
			SetPathToRead();
			ProbablyLoadsSave(0);
			SetPathToWrite();

			PrintDebug("Reloaded Savefile\n");
			debugMessage = "Reloaded Savefile";
			debugMessageTime = 100;
		}

		oldMenu = CurrentMenu;

		if (!hasInit && debugMessageTime != 0)
		{
			SetupDebugInfo(18, 0xFF00FFAA);
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 1), "WARNING:");
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 2), debugMessage.c_str());

			debugMessageTime--;
		}
		else if (debugMessageTime != 0)
		{
			SetupDebugInfo(18, 0xFF00FFAA);
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 1), "SA2 Quick Save Reload");
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 3), debugMessage.c_str());
			if (extraDebugMessageTime != 0)
				HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 4), extraDebugMessage.c_str());

			debugMessageTime--;
		}
	}

	__declspec(dllexport) void __cdecl OnInput()
	{
		
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