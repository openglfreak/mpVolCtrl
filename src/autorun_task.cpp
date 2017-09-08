#include "unicode.h"

#include <Windows.h>
#include <tchar.h>
#include <wchar.h>
#include <WTypes.h>
#include <CommCtrl.h>
#include <comdef.h>
#include <taskschd.h>

#define SECURITY_WIN32
#include <Security.h>

#include <string>
#include <vector>

#include "resource.h"
#include "auto_cleanup.hpp"
#include "errors.hpp"
#include "autorun_task.hpp"

static const OLECHAR AUTORUN_TASK_NAME[] = OLESTR("mpVolCtrl Autorun");
static BSTR cachedUserName = NULL;

BSTR get_user_name_bstr()
{
	static BSTR cached_username = NULL;
	static AutoDeleter<BSTR&, void (STDAPICALLTYPE *)(BSTR)> cached_username_deleter(cached_username, SysFreeString);
	if (cached_username != NULL)
		return cached_username;

	ULONG len = 64;
	std::vector<OLECHAR> username_vect(len);
	if (!GetUserNameExW(NameSamCompatible, &username_vect[0], &len))
		do {
			DWORD error = GetLastError();
			if (error == ERROR_MORE_DATA)
			{
				username_vect.resize(len);
				if (GetUserNameExW(NameSamCompatible, &username_vect[0], &len))
					break;
				error = GetLastError();
			}
			ShowErrorMessage(error, _T("GetUserNameExW error"));
			return NULL;
		} while (false);

	return cached_username = SysAllocStringLen(&username_vect[0], len);
}

BSTR get_exe_file_name_bstr()
{
	static BSTR cached_filename = NULL;
	static AutoDeleter<BSTR&, void (STDAPICALLTYPE *)(BSTR)> cached_filename_deleter(cached_filename, SysFreeString);
	if (cached_filename != NULL)
		return cached_filename;

	std::vector<WCHAR> filename_vect(MAX_PATH + 1);
	DWORD len = GetModuleFileNameW(NULL, &filename_vect[0], MAX_PATH + 1);
	filename_vect.resize(len);
	if (len == 0)
		ShowErrorMessage(GetLastError(), _T("GetModuleFileNameW error"));
	else if (len == MAX_PATH + 1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		ShowErrorMessage(ERROR_INSUFFICIENT_BUFFER, _T("GetModuleFileNameW error"));
	else
		return cached_filename = SysAllocStringLen(&filename_vect[0], len);
	return NULL;
}

bool create_logon_task(ITaskService* iTaskScheduler, ITaskFolder* iTaskFolder, IRegisteredTask*& out)
{
	ITaskDefinition* iAutorunTaskDefinition;
	HRESULT hResult = iTaskScheduler->NewTask(0, &iAutorunTaskDefinition);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskService::NewTask error"));
		return false;
	}
	AutoReleaser<ITaskDefinition> iAutorunTaskDefinitionReleaser(iAutorunTaskDefinition);

	BSTR tmp;
	{
		IRegistrationInfo* iAutorunRegistrationInfo;
		hResult = iAutorunTaskDefinition->get_RegistrationInfo(&iAutorunRegistrationInfo);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IRegistrationInfo::get_RegistrationInfo error"));
			return false;
		}
		AutoReleaser<IRegistrationInfo> iAutorunRegistrationInfoReleaser(iAutorunRegistrationInfo);

		tmp = SysAllocString(OLESTR("Torge Matthies"));
		iAutorunRegistrationInfo->put_Author(tmp);
		SysFreeString(tmp);

		tmp = SysAllocString(OLESTR("Starts Media Player Volume Control on logon"));
		iAutorunRegistrationInfo->put_Description(tmp);
		SysFreeString(tmp);

		tmp = SysAllocString(OLESTR("Media Player Volume Control"));
		iAutorunRegistrationInfo->put_Source(tmp);
		SysFreeString(tmp);

		tmp = SysAllocString(PRODUCT_NAME_WIDE VERSION_STRING_WIDE);
		iAutorunRegistrationInfo->put_Source(tmp);
		SysFreeString(tmp);
	}

	VARIANT username;
	{
		ITriggerCollection* iTriggers;
		hResult = iAutorunTaskDefinition->get_Triggers(&iTriggers);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskDefinition::get_Triggers error"));
			return false;
		}
		AutoReleaser<ITriggerCollection> iAutorunTriggersReleaser(iTriggers);

		ITrigger* iAutorunTrigger;
		hResult = iTriggers->Create(TASK_TRIGGER_LOGON, &iAutorunTrigger);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITriggerCollection::Create error"));
			return false;
		}
		AutoReleaser<ITrigger> iAutorunTriggerReleaser(iAutorunTrigger);

		tmp = SysAllocString(OLESTR("Logon"));
		iAutorunTrigger->put_Id(tmp);
		SysFreeString(tmp);

		ILogonTrigger* iLogonTrigger;
		iAutorunTrigger->QueryInterface(IID_ILogonTrigger, (void**)&iLogonTrigger);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITrigger::QueryInterface[ILogonTrigger] error"));
			return false;
		}
		AutoReleaser<ILogonTrigger> iLogonTriggerReleaser(iLogonTrigger);

		if (!(username.bstrVal = get_user_name_bstr()))
			return false;
		username.vt = VT_BSTR;
		iLogonTrigger->put_UserId(username.bstrVal);
	}

	{
		IActionCollection* iActions;
		hResult = iAutorunTaskDefinition->get_Actions(&iActions);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskDefinition::get_Actions error"));
			return false;
		}
		AutoReleaser<IActionCollection> iActionsReleaser(iActions);

		IAction* iAction;
		hResult = iActions->Create(TASK_ACTION_EXEC, &iAction);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IActionCollection::Create error"));
			return false;
		}
		AutoReleaser<IAction> iActionReleaser(iAction);

		tmp = SysAllocString(OLESTR("Start mpVolCtrl"));
		iAction->put_Id(tmp);
		SysFreeString(tmp);

		IExecAction* iExecAction;
		hResult = iAction->QueryInterface(IID_IExecAction, (void**)&iExecAction);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IAction::QueryInterface[IID_IExecAction] error"));
			return false;
		}
		AutoReleaser<IExecAction> iExecActionReleaser(iExecAction);

		if (!(tmp = get_exe_file_name_bstr()))
			return false;
		hResult = iExecAction->put_Path(tmp);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IExecAction::put_Path error"));
			return false;
		}
	}

	{
		ITaskSettings* iSettings;
		hResult = iAutorunTaskDefinition->get_Settings(&iSettings);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskDefinition::get_Settings error"));
			return false;
		}
		AutoReleaser<ITaskSettings> iSettingsReleaser(iSettings);

		hResult = iSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskSettings::put_DisallowStartIfOnBatteries error"));
			return false;
		}
		hResult = iSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskSettings::put_StopIfGoingOnBatteries error"));
			return false;
		}
		tmp = SysAllocString(OLESTR("PT0S"));
		hResult = iSettings->put_ExecutionTimeLimit(tmp);
		SysFreeString(tmp);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskSettings::put_ExecutionTimeLimit error"));
			return false;
		}
		hResult = iSettings->put_StartWhenAvailable(VARIANT_TRUE);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskSettings::put_StartWhenAvailable error"));
			return false;
		}
	}

	tmp = SysAllocString(AUTORUN_TASK_NAME);
	hResult = iTaskFolder->RegisterTaskDefinition(tmp, iAutorunTaskDefinition, TASK_CREATE, username, _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(OLESTR("")), &out);
	SysFreeString(tmp);
	if (SUCCEEDED(hResult))
		return true;
	ShowErrorMessage(hResult, _T("ITaskFolder::RegisterTaskDefinition error"));
	return false;
}

bool validate_autorun_task(ITaskFolder* iTaskFolder, IRegisteredTask* iAutorunTask)
{
	ITaskDefinition* iAutorunTaskDefinition;
	HRESULT hResult = iAutorunTask->get_Definition(&iAutorunTaskDefinition);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskDefinition::get_Definition error"));
		return false;
	}
	AutoReleaser<ITaskDefinition> iAutorunTaskDefinitionReleaser(iAutorunTaskDefinition);

	IActionCollection* iActions;
	hResult = iAutorunTaskDefinition->get_Actions(&iActions);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskDefinition::get_Actions error"));
		return false;
	}
	AutoReleaser<IActionCollection> iActionsReleaser(iActions);

	long count;
	hResult = iActions->get_Count(&count);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("IActionCollection::get_Count error"));
		return false;
	}
	for (long i = 1; i <= count; ++i)
	{
		IAction* iAction;
		hResult = iActions->get_Item(i, &iAction);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IActionCollection::get_Item error"));
			return false;
		}
		AutoReleaser<IAction> iActionReleaser(iAction);

		BSTR tmp;
		hResult = iAction->get_Id(&tmp);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IAction::get_Id error"));
			return false;
		}
		if (tmp == NULL || wcscmp(OLESTR("Start mpVolCtrl"), tmp) != 0)
			continue;

		TASK_ACTION_TYPE type;
		hResult = iAction->get_Type(&type);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IAction::get_Type error"));
			return false;
		}
		if (type != TASK_ACTION_EXEC)
			continue;

		IExecAction* iExecAction;
		hResult = iAction->QueryInterface(IID_IExecAction, (void**)&iExecAction);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IAction::QueryInterface error"));
			return false;
		}
		AutoReleaser<IExecAction> iExecActionReleaser(iExecAction);

		hResult = iExecAction->get_Path(&tmp);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IExecAction::get_Path error"));
			return false;
		}
		if (SysStringLen(get_exe_file_name_bstr()) >= SysStringLen(tmp) && (SysStringLen(get_exe_file_name_bstr()) != SysStringLen(tmp) || wcscmp(get_exe_file_name_bstr(), tmp) == 0))
			break;

		hResult = iExecAction->put_Path(get_exe_file_name_bstr());
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IExecAction::put_Path error"));
			return false;
		}
		hResult = iAutorunTaskDefinition->put_Actions(iActions);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskDefinition::put_Actions error"));
			return false;
		}
		tmp = SysAllocString(AUTORUN_TASK_NAME);
		VARIANT username;
		username.vt = VT_BSTR;
		username.bstrVal = get_user_name_bstr();
		hResult = iTaskFolder->RegisterTaskDefinition(tmp, iAutorunTaskDefinition, TASK_UPDATE, username, _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(OLESTR("")), &iAutorunTask);
		SysFreeString(tmp);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("ITaskFolder::RegisterTaskDefinition error"));
			return false;
		}
	}
	return true;
}

bool set_autorun_state(bool enabled)
{
	ITaskService* iTaskScheduler;
	HRESULT hResult = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_ALL, IID_ITaskService, (LPVOID*)&iTaskScheduler);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("CoCreateInstance[ITaskService] error"));
		return false;
	}
	AutoReleaser<ITaskService> iTaskSchedulerReleaser(iTaskScheduler);

	hResult = iTaskScheduler->Connect(VARIANT(), VARIANT(), VARIANT(), VARIANT());
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskService::Connect error"));
		return false;
	}

	BSTR tmp = SysAllocString(OLESTR("\\"));
	ITaskFolder* iRootTaskFolder;
	hResult = iTaskScheduler->GetFolder(tmp, &iRootTaskFolder);
	SysFreeString(tmp);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskService::GetFolder error"));
		return false;
	}
	AutoReleaser<ITaskFolder> iRootTaskFolderReleaser(iRootTaskFolder);

	IRegisteredTask* iAutorunTask;
	tmp = SysAllocString((std::basic_string<OLECHAR>(OLESTR("\\")) += AUTORUN_TASK_NAME).c_str());
	hResult = iRootTaskFolder->GetTask(tmp, &iAutorunTask);
	AutoReleaser<IRegisteredTask> iAutorunTaskReleaser(iAutorunTask, !SUCCEEDED(hResult));
	SysFreeString(tmp);
	if (iAutorunTaskReleaser)
		validate_autorun_task(iRootTaskFolder, iAutorunTask);
	else
		if (!create_logon_task(iTaskScheduler, iRootTaskFolder, iAutorunTask))
			return false;
		else if (enabled)
			return true;

	hResult = iAutorunTask->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("IRegisteredTask::put_Enabled error"));
		return false;
	}
	return true;
}

bool get_autorun_state(bool& enabled)
{
	ITaskService* iTaskScheduler;
	HRESULT hResult = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_ALL, IID_ITaskService, (LPVOID*)&iTaskScheduler);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("CoCreateInstance[ITaskService] error"));
		return false;
	}
	AutoReleaser<ITaskService> iTaskSchedulerReleaser(iTaskScheduler);

	hResult = iTaskScheduler->Connect(VARIANT(), VARIANT(), VARIANT(), VARIANT());
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskService::Connect error"));
		return false;
	}

	BSTR tmp = SysAllocString(OLESTR("\\"));
	ITaskFolder* iRootTaskFolder;
	hResult = iTaskScheduler->GetFolder(tmp, &iRootTaskFolder);
	SysFreeString(tmp);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("ITaskService::GetFolder error"));
		return false;
	}
	AutoReleaser<ITaskFolder> iRootTaskFolderReleaser(iRootTaskFolder);

	IRegisteredTask* iAutorunTask;
	tmp = SysAllocString((std::basic_string<OLECHAR>(OLESTR("\\")) += AUTORUN_TASK_NAME).c_str());
	hResult = iRootTaskFolder->GetTask(tmp, &iAutorunTask);
	AutoReleaser<IRegisteredTask> iAutorunTaskReleaser(iAutorunTask, !SUCCEEDED(hResult));
	SysFreeString(tmp);
	if (iAutorunTaskReleaser)
	{
		if (!validate_autorun_task(iRootTaskFolder, iAutorunTask))
			return false;
		VARIANT_BOOL b;
		iAutorunTask->get_Enabled(&b);
		enabled = !!b;
	}
	else
		enabled = false;
	return true;
}
