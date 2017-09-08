#pragma once
#ifndef __AUTORUN_TASK_HPP__
#define __AUTORUN_TASK_HPP__

#include <WTypes.h>
#include <taskschd.h>

BSTR get_user_name_bstr();
BSTR get_exe_file_name_bstr();
bool create_logon_task(ITaskService* iTaskScheduler, ITaskFolder* iTaskFolder, IRegisteredTask*& out);
bool validate_autorun_task(ITaskFolder* iTaskFolder, IRegisteredTask* iAutorunTask);
bool set_autorun_state(bool enabled);
bool get_autorun_state(bool& enabled);

#endif // __AUTORUN_TASK_HPP__
