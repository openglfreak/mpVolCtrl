#include "unicode.h"
#include <assert.h>

#include <algorithm>

#include <initguid.h>
#include <Windows.h>
#include <tchar.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <Psapi.h>

#include "auto_cleanup.hpp"
#include "errors.hpp"
#include "volume_control.hpp"

#if _MSC_VER
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioSessionManager2 = __uuidof(IAudioSessionManager2);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioSessionControl2 = __uuidof(IAudioSessionControl2);
#endif

class AudioSesionInterfaceVolumeControlProvider : public MediaPlayerVolumeControlProvider
{
protected:
	std::vector<std::basic_string<TCHAR>> processNames;
	IMMDeviceEnumerator* iMMDevEnum;
public:
	~AudioSesionInterfaceVolumeControlProvider()
	{
		if (iMMDevEnum)
			iMMDevEnum->Release();
	}

	void register_process_name(std::basic_string<TCHAR> const& name)
	{
		processNames.push_back(name);
	}
private:
	template<typename UnaryFunction>
	bool apply_to_all(UnaryFunction f)
	{
		static TCHAR processPath[MAX_PATH + 1];
		HRESULT hResult;

		if (!iMMDevEnum)
		{
			hResult = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (LPVOID*)&iMMDevEnum);
			if (!SUCCEEDED(hResult))
			{
				ShowErrorMessage(hResult, _T("CoCreateInstance[IMMDeviceEnumerator] error"));
				return false;
			}
		}

		IMMDeviceCollection* iMMDevColl;
		hResult = iMMDevEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &iMMDevColl);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IMMDeviceEnumerator::EnumAudioEndpoints error"));
			return false;
		}
		AutoReleaser<IMMDeviceCollection> iMMDevCollReleaser(iMMDevColl);

		UINT deviceCount;
		// Never trust anything
		hResult = iMMDevColl->GetCount(&deviceCount);
		if (!SUCCEEDED(hResult))
		{
			ShowErrorMessage(hResult, _T("IMMDeviceCollection::GetCount error"));
			return false;
		}
		for (UINT i = 0; i < deviceCount; ++i)
		{
			IMMDevice* iMMDevice;
			hResult = iMMDevColl->Item(i, &iMMDevice);
			if (!SUCCEEDED(hResult))
				continue;
			AutoReleaser<IMMDevice> iMMDeviceReleaser(iMMDevice);

			IAudioSessionManager2* iAudioSessMgr;
			hResult = iMMDevice->Activate(IID_IAudioSessionManager2, CLSCTX_ALL, NULL, (void**)&iAudioSessMgr);
			if (!SUCCEEDED(hResult))
				continue;
			AutoReleaser<IAudioSessionManager2> iAudioSessMgrReleaser(iAudioSessMgr);

			IAudioSessionEnumerator* iAudioSessEnum;
			hResult = iAudioSessMgr->GetSessionEnumerator(&iAudioSessEnum);
			if (!SUCCEEDED(hResult))
				continue;
			AutoReleaser<IAudioSessionEnumerator> iAudioSessEnumReleaser(iAudioSessEnum);

			int sessionCount;
			// Really, don't EVER trust ANYTHING!
			hResult = iAudioSessEnum->GetCount(&sessionCount);
			if (!SUCCEEDED(hResult))
				continue;
			for (int j = 0; j < sessionCount; ++j)
			{
				IAudioSessionControl* iAudioSessCtrl;
				hResult = iAudioSessEnum->GetSession(j, &iAudioSessCtrl);
				if (!SUCCEEDED(hResult))
					continue;
				AutoReleaser<IAudioSessionControl> iAudioSessCtrlReleaser(iAudioSessCtrl);

				IAudioSessionControl2* iAudioSessCtrl2;
				hResult = iAudioSessCtrl->QueryInterface(IID_IAudioSessionControl2, (void**)&iAudioSessCtrl2);
				if (!SUCCEEDED(hResult))
					continue;
				AutoReleaser<IAudioSessionControl2> iAudioSessCtrl2Releaser(iAudioSessCtrl2);

				DWORD processId;
				hResult = iAudioSessCtrl2->GetProcessId(&processId);
				if (!SUCCEEDED(hResult))
					continue;

				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
				if (!hProcess)
					continue;
				DWORD len = GetProcessImageFileName(hProcess, processPath, MAX_PATH + 1);
				CloseHandle(hProcess);
				if (len == 0)
					continue;
				TCHAR* lastDirSep = std::find(std::make_reverse_iterator(&processPath[len]), std::make_reverse_iterator(&processPath[0]), _T('\\')).base();
				std::basic_string<TCHAR> processName(lastDirSep, &processPath[len]);
				if (std::find(processNames.begin(), processNames.end(), processName) != processNames.end())
					f(iAudioSessCtrl2);
			}
		}

		return true;
	}

	class ChangeVolume
	{
	private:
		float delta;
	public:
		MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS status;

		ChangeVolume(float delta) : delta(delta), status(STATUS_NOT_FOUND) { }

		void operator()(IAudioSessionControl2* iAudioSessCtrl)
		{
			status = STATUS_FOUND;
			ISimpleAudioVolume* iAudioVolume;
			HRESULT hResult = iAudioSessCtrl->QueryInterface(IID_ISimpleAudioVolume, (void**)&iAudioVolume);
			if (!SUCCEEDED(hResult))
				return;
			AutoReleaser<ISimpleAudioVolume> iAudioVolumeReleaser(iAudioVolume);
			float volume;
			iAudioVolume->GetMasterVolume(&volume);
			iAudioVolume->SetMasterVolume(std::min<float>(std::max<float>(volume += delta, 0.f), 1.f), NULL);
		}
	};

	virtual VOLUME_CHANGE_STATUS change_volume(float delta)
	{
		ChangeVolume cv(delta);
		if (!apply_to_all<ChangeVolume&>(cv))
			return STATUS_ERROR;
		return cv.status;
	}
};

/*
class VLCMediaPlayerVolumeControlProvider;
*/

std::vector<MediaPlayerVolumeControlProvider*>* volumeControlProvidersPtr = NULL;

bool add_volume_control(MediaPlayerVolumeControlProvider* vcp)
{
	if (!volumeControlProvidersPtr && !(volumeControlProvidersPtr = new std::vector<MediaPlayerVolumeControlProvider*>()))
		return false;
	volumeControlProvidersPtr->push_back(vcp);
	return true;
}
bool remove_volume_control(MediaPlayerVolumeControlProvider* vcp)
{
	if (volumeControlProvidersPtr)
	{
		std::vector<MediaPlayerVolumeControlProvider*>::iterator end = volumeControlProvidersPtr->end(), new_end = std::remove(volumeControlProvidersPtr->begin(), end, vcp);
		if (new_end != end)
		{
			volumeControlProvidersPtr->erase(new_end);
			return true;
		}
	}
	return false;
}
void delete_volume_controls()
{
	for (std::vector<MediaPlayerVolumeControlProvider*>::iterator start = volumeControlProvidersPtr->begin(), end = volumeControlProvidersPtr->end(); start != end; ++start)
		delete *start;
	volumeControlProvidersPtr->clear();
}

MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS volume_change(float amount)
{
	MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS ret = MediaPlayerVolumeControlProvider::STATUS_NOT_FOUND;
	for (std::vector<MediaPlayerVolumeControlProvider*>::iterator start = volumeControlProvidersPtr->begin(), end = volumeControlProvidersPtr->end(); start != end; ++start)
	{
		MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS tmp = (*start)->volume_change(amount);
		if (tmp == MediaPlayerVolumeControlProvider::STATUS_FOUND)
			ret = MediaPlayerVolumeControlProvider::STATUS_FOUND;
	}
	return ret;
}

struct __dummy {
	__dummy()
	{
		AudioSesionInterfaceVolumeControlProvider* audioSessIfaceVolCtrlProvider(new AudioSesionInterfaceVolumeControlProvider());
		audioSessIfaceVolCtrlProvider->register_process_name(std::basic_string<TCHAR>(_T("wmplayer.exe")));
		add_volume_control(audioSessIfaceVolCtrlProvider);
	}
	~__dummy()
	{
		assert(volumeControlProvidersPtr != NULL);
		delete volumeControlProvidersPtr;
	}
} __dummy_inst;
