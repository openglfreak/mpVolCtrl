#pragma once
#ifndef __MPVC_CONFIG_HPP__
#define __MPVC_CONFIG_HPP__

#include "unicode.h"

#include <stdlib.h>
#include <tchar.h>

#include <fstream>
#include <string>

#include "config.hpp"

class MPVCConfig : public config::ConfigIO<_TCHAR>
{
private:
#if defined(_MSC_VER) || !UNICODE
	typedef std::basic_string<TCHAR> path_type;
#else
	typedef std::string path_type;
#endif
	path_type configPath;
public:
	bool disabled;
	bool invisible;

	bool startHidden;

	MPVCConfig()
	{
		config::ConfigIO<_TCHAR>::add_option(_T("Disabled"), _T("Whether the Media Keys redirection is disabled or enabled on start. true for disabled and false for enabled"), disabled);
		config::ConfigIO<_TCHAR>::add_option(_T("StartHidden"), _T("Whether the Notification Area icon is shown or not. true for hidden and false for not hidden"), startHidden);
	}

	int get_config_path()
	{
		std::basic_string<TCHAR> tmp(MAX_PATH, _T('\0'));
		while (!SUCCEEDED(SHGetSpecialFolderPath(NULL, &tmp[0], CSIDL_APPDATA, TRUE)) && !SUCCEEDED(SHGetSpecialFolderPath(NULL, &tmp[0], CSIDL_LOCAL_APPDATA, TRUE)) && !SUCCEEDED(SHGetSpecialFolderPath(NULL, &tmp[0], CSIDL_MYDOCUMENTS, TRUE)))
		{
			int o = MessageBox(NULL, _T("Couldn't determine config folder"), _T("SHGetSpecialFolderPath error"), MB_ABORTRETRYIGNORE | MB_DEFBUTTON2);
			if (o == IDABORT)
				return 0;
			else if (o == IDRETRY)
				continue;
			else if (o == IDIGNORE)
				return -1;
		}

		tmp.resize(_tcslen(&tmp[0]));
		tmp.append(_T("\\mpVolCtrl"));
		CreateDirectory(tmp.c_str(), NULL);
		tmp.append(_T("\\config.txt"));

#if defined(_MSC_VER) || !UNICODE
		configPath = tmp;
#else
		configPath.clear();
		configPath.resize(wcstombs(0, tmp.c_str(), -1) + 1);
		size_t nCharsConverted;
		if ((nCharsConverted = wcstombs(&configPath[0], tmp.c_str(), tmp.length())) != (size_t)-1)
			configPath.resize(nCharsConverted);
		else
			configPath.clear();
#endif
		return 1;
	}

	bool read_config(bool writeIfMissing = true)
	{
		if (configPath.empty())
			get_config_path();

		std::basic_fstream<_TCHAR> fs;
		fs.open(configPath, std::basic_fstream<_TCHAR>::in);
		if (fs.fail())
		{
			if (writeIfMissing)
			{
				fs.open(configPath, std::basic_fstream<_TCHAR>::out);
				if (!fs.fail())
				{
					fs << std::noskipws << "# Media Player Volume Control config" << std::endl << "# Generated by Media Player Volume Control " VERSION_STRING << std::endl << std::endl;
					config::ConfigIO<_TCHAR>::generate_config(std::ostreambuf_iterator<_TCHAR>(fs));
				}
			}
		}
		else
			config::ConfigIO<_TCHAR>::parse_config(std::istreambuf_iterator<_TCHAR>(fs >> std::noskipws), std::istreambuf_iterator<_TCHAR>());
		if (fs.is_open())
			fs.close();

		invisible = startHidden;
		return true;
	}

	bool write_config()
	{
		std::basic_fstream<_TCHAR> fs;
		fs.open(configPath, std::basic_fstream<_TCHAR>::out);
		if (!fs.fail())
		{
			fs << std::noskipws << "# Media Player Volume Control config" << std::endl << "# Generated by Media Player Volume Control " VERSION_STRING << std::endl << std::endl;
			config::ConfigIO<_TCHAR>::generate_config(std::ostreambuf_iterator<_TCHAR>(fs));
			fs.close();
			return true;
		}
		return false;
	}
};

#endif // __MPVC_CONFIG_HPP__
