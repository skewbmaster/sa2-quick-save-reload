#include "pch.h"
#include <iostream>
#include <string>
#include <filesystem>

#define DEBUGTEXT false
#define DEFAULT_MESSAGE_TIME 480
#define SA2_SAVE_SIZE 24576

bool enabled;
bool usePromptVer;
bool reloadDelay;
bool forceReload;
bool deleteMainFile;
bool deleteChao;
static char* newSavePath;
static char* slotSavePath;
static std::string modPath;

int oldMenu;

bool hasInit;
bool hasChosenFile;
static std::string debugMessage;
static std::string extraDebugMessage;
int debugMessageTime;
int extraDebugMessageTime;
int resetTimer;

static int* accessibleMemory;

HelperFunctions HelperFunctionsGlobal;

void SetPathToRead() { memcpy_s(&CurrentSavePath, 4, &newSavePath, 4); }
void SetPathToWrite() { memcpy_s(&CurrentSavePath, 4, &slotSavePath, 4); }
void SetPathToNormal()
{
	int resourceGD1Pointer = 0x173D01C;
	WriteData(&CurrentSavePath, &resourceGD1Pointer, 4);
}

void ReloadSaveFile();
void ReloadSaveMenu();
void SetupDebugInfo(int scaleText, int colour);

FunctionPointer(int, LoadSaveInGame, (char), 0x445100);
VoidFunc(ResetTempAnimalCount, 0x4872C0);
VoidFunc(Reset2PMenuOptionsAndOtherThings, 0x666790);

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		HelperFunctionsGlobal = helperFunctions;
		modPath = std::string(path);
		hasInit = false;

		// Config start
		const IniFile* configFile = new IniFile(modPath + "\\config.ini");

		enabled = configFile->getBool("QSRSettings", "Enabled", false);
		usePromptVer = configFile->getBool("QSRSettings", "UsePromptVersion", false);
		std::string saveFilePath = configFile->getString("QSRSettings", "SaveFilePath", "");
		int saveNum = configFile->getInt("QSRSettings", "SaveNum", 10);
		reloadDelay = configFile->getBool("QSRSettings", "ReloadDelay", false);
		forceReload = configFile->getBool("QSRSettings", "ForceReload", false);
		deleteMainFile = configFile->getBool("QSRSettings", "DeleteMainFile", true);
		deleteChao = configFile->getBool("QSRSettings", "DeleteChao", false);

		delete configFile;
		// Config End

		if (!enabled)
		{
			debugMessage = "Quick Save Reloader is disabled";
			debugMessageTime = DEFAULT_MESSAGE_TIME;
			return;
		}

		newSavePath = (char*)malloc(256); // Allocate string for the game to read our file
		slotSavePath = (char*)malloc(64); // Allocate string for writing savedata
		if (!newSavePath || !slotSavePath)
		{
			debugMessage = "Couldn't allocate memory for strings, somehow out of memory??";
			debugMessageTime = DEFAULT_MESSAGE_TIME;
			return;
		}

		sprintf_s(slotSavePath, 39, "./resource/gd_PC/SAVEDATA/SONIC2B__S%02d", saveNum);

		std::string loadedPath;
		if (saveFilePath != "")
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
			strcpy_s(newSavePath, loadedPath.length() + 1, loadedPath.c_str());
			SetPathToRead();
			hasChosenFile = false;
		}
		else if (!usePromptVer)
		{
			std::string loadedPath = ".\\" + modPath + "\\premadeSaves\\Clean"; // Set reload path to one of the ones inside premade saves
			strcpy_s(newSavePath, loadedPath.length() + 1, loadedPath.c_str());
			SetPathToRead();
			hasChosenFile = false;
		}
		else
		{
			hasChosenFile = true;
		}

		extraDebugMessage = "Using File Number " + std::to_string(saveNum);
		extraDebugMessageTime = 160;
		debugMessageTime = 160;
		hasInit = true;
		resetTimer = 0;

		accessibleMemory = (int*) malloc(64);
		if (!accessibleMemory)
		{
			PrintDebug("QSR accessible memory couldn't be allocated for some reason\n");
			return;
		}
		int accessMemPAsInt = reinterpret_cast<int>(accessibleMemory);
		WriteData((void*) 0x454884, &accessMemPAsInt, 4); // 0x454884 has some unused memory which we will write our pointer to
		*accessibleMemory = 0;
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		if (!usePromptVer)
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
			HelperFunctionsGlobal.DisplayDebugString(NJM_LOCATION(1, 2), debugMessage.c_str());

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
			if (usePromptVer)
			{
				ReloadSaveMenu();
			}
			else
			{
				ReloadSaveFile();
			}
		}
		else if (!forceReload && CurrentMenu == Menus_TitleScreen)
		{
#if DEBUGTEXT
			extraDebugMessage = "Press Y to reload Save Menu";
			extraDebugMessageTime = 1;
#endif
			PDS_PERIPHERAL* controller = ControllerPointers[0];
			
			if (controller && ((controller->on & Buttons_Y && reloadDelay) || controller->press & Buttons_Y)) // || keyboard handling maybe? Not possible cos sa2 doesn't map keyboard keys :)
			{
				resetTimer++;
				if (resetTimer == 15 || !reloadDelay)
				{
					if (usePromptVer)
					{
						ReloadSaveMenu();
						debugMessage = "Reloaded File Prompt";
					}
					else
					{
						ReloadSaveFile();
						debugMessage = "Reloaded Save File";
					}
					debugMessageTime = 60;
					(*accessibleMemory)++;
				}
			}
			else
			{
				resetTimer = 0;
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

void DeleteMainSave()
{
	std::filesystem::path savefilepath;
	//PrintDebug(slotSavePath);
	//PrintDebug((".\\" + std::string(modPath) + "\\backups\\" + (slotSavePath + 26)).c_str());
	if (std::filesystem::exists(slotSavePath))
	{
		std::error_code exception;

		if (std::filesystem::copy_file(slotSavePath, ".\\" + modPath + "\\backups\\" + (slotSavePath + 26), std::filesystem::copy_options::overwrite_existing, exception))
		{
			PrintDebug("Copied Savefile for backup\n");
		}
		else
		{
			PrintDebug(("Couldn't copy Savefile. " + exception.message()).c_str());
		}

		if (std::filesystem::remove(slotSavePath, exception))
		{
			PrintDebug("Deleted Savefile\n");
		}
		else
		{
			PrintDebug("Couldn't delete Savefile\n");
		}
	}
	else
	{
		PrintDebug("Savefile doesn't exist\n");
	}
}

void DeleteChaoSave()
{
	if (std::filesystem::exists("resource/gd_PC/SAVEDATA/SONIC2B__ALF"))
	{
		std::error_code exception;

		if (std::filesystem::copy_file("resource/gd_PC/SAVEDATA/SONIC2B__ALF", ".\\" + modPath + "\\backups\\SONIC2B__ALF", std::filesystem::copy_options::overwrite_existing, exception))
		{
			PrintDebug("Copied Chao file for backup\n");
		}
		else
		{
			PrintDebug(("Couldn't copy Chao file" + exception.message()).c_str());
		}

		if (std::filesystem::remove("resource/gd_PC/SAVEDATA/SONIC2B__ALF", exception))
		{
			PrintDebug("Deleted Chao file\n");
			//debugMessage = "Reloaded Save menu and Deleted Chao file";
		}
		else
		{
			PrintDebug("Couldn't delete Chao file\n");
			//debugMessage = "Reloaded Save menu but couldn't delete Chao file";
		}
	}
	else
	{
		PrintDebug("Chao file doesn't exist\n");
		//debugMessage = "Reloaded Save menu but Chao file doesn't exist";
	}
}

void ReloadSaveMenu()
{
	WriteData<4>((void*) 0x173D06C, -1);
	PrintDebug("Reloaded Save menu\n");

#if DEBUGTEXT
	debugMessage = "Reloaded Save Menu";
	debugMessageTime = 100;
#endif

	if (deleteMainFile)
	{
		DeleteMainSave();
	}

	if (deleteChao)
	{
		DeleteChaoSave();
	}
}

void ReloadSaveFile()
{
	if (!std::filesystem::exists(newSavePath))
	{
		PrintDebug("Reload file doesn't exist anymore\n");
#if DEBUGTEXT
		debugMessage = "Reload file doesn't exist anymore";
		debugMessageTime = 100;
#endif
		return;
	}

	SetPathToRead();
	LoadSaveInGame(0);
	ResetTempAnimalCount();
	Reset2PMenuOptionsAndOtherThings();
	SetPathToWrite();

	PrintDebug("Reloaded Savefile\n");
#if DEBUGTEXT
	debugMessage = "Reloaded Savefile";
	debugMessageTime = 100;
#endif

	if (deleteChao)
	{
		DeleteChaoSave();
	}
}

