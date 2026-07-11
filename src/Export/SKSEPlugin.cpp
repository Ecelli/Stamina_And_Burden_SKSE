#include "Burden/BurdenManager.h"
#include "Data/ModObjectManager.h"
#include "Hooks/Hooks.h"
#include "Papyrus/Papyrus.h"
#include "Settings/Params/SettingsOverride.h"
#include "Serialization/Serde.h"
#include "Settings/INI/INISettings.h"
#include "Settings/JSON/JSONSettings.h"
#include "Common/Utils.h"
#include "API/PerkEntryPointExtenderAPI.h"
#include "Hooks/DenyHooks.h"

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		SECTION_SEPARATOR;
		Regen::OverrideGameSettings();
		SECTION_SEPARATOR;

		if (!PerkEntryPointExtenderAPI::RequestInterface()) {
			logger::warn("  >PEPE not found — perk categories will be inactive");
		} else {
			logger::info("  >PEPE interface resolved");
		}

		SECTION_SEPARATOR;
		if (!Data::PreloadModObjects()) {
			SKSE::stl::report_and_fail("Failed to preload mod objects. Check the log for more information."sv);
		}

		SECTION_SEPARATOR;
		Hooks::PlayerNormalAttackDenyHook::Install();

		SECTION_SEPARATOR;
		logger::info("Finished startup tasks, enjoy your game!"sv);
		Settings::JSON::Holder::GetSingleton()->Release();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v{};

		v.PluginVersion(Plugin::VERSION);
		v.PluginName(Plugin::NAME);
		v.AuthorName("Ecelli"sv);
		v.UsesAddressLibrary();
		v.UsesUpdatedStructs();

		return v;
	}();
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < MIN_RUNTIME) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
	}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse)
{
	SKSE::Init(a_skse);
	logger::info("Author: Ecelli"sv);
	SECTION_SEPARATOR;

	const auto ver = a_skse->RuntimeVersion();
	if (ver < MIN_RUNTIME) {
		return false;
	}

	logger::info("Performing startup tasks..."sv);

	if (!Settings::INI::Read()) {
		SKSE::stl::report_and_fail("Failed to load INI settings. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;
	if (!Hooks::Install()) {
		SKSE::stl::report_and_fail("Failed to install hooks. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;

	SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	logger::info("Setting up serialization system..."sv);
	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);
	logger::info("  >Registered necessary functions."sv);
	SECTION_SEPARATOR;


	if (!Settings::JSON::Preload()) {
		SKSE::stl::report_and_fail("Failed to preload JSON configs. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;
	return true;
}
