#include <string>
#include <stdlib.h>

#include "../USB.h"

extern HINSTANCE hInst;
std::wstring IniDir;
std::wstring LogDir;

void CALLBACK USBsetSettingsDir( const char* dir )
{
	fprintf(stderr, "USBsetSettingsDir: %s\n", dir);
	wchar_t dst[4096] = {0};
	size_t num = 0;
	mbstowcs_s(&num, dst, dir, ARRAYSIZE(dst));
	IniDir = dst;
}

void CALLBACK USBsetLogDir( const char* dir )
{
	printf("USBsetLogDir: %s\n", dir);
	wchar_t dst[4096] = {0};
	size_t num = 0;
	mbstowcs_s(&num, dst, dir, ARRAYSIZE(dst));
	LogDir = dst;
}

void GetIniFile(std::wstring &iniFile)
{
	iniFile.clear();
	if(!IniDir.length()) {
		WCHAR tmp[MAX_PATH] = {0};
		GetModuleFileName(GetModuleHandle((LPWSTR)hInst), tmp, MAX_PATH);

		std::wstring path(tmp);
		unsigned last = path.find_last_of(L"\\");
		iniFile = path.substr(0, last);
		iniFile.append(L"\\inis");
		CreateDirectory(iniFile.c_str(), NULL);
		iniFile.append(L"\\USBqemu-wheel.ini");
	} else {
		iniFile.append(IniDir);
		iniFile.append(L"USBqemu-wheel.ini");
	}
}

void SaveConfig()
{
	Config *Conf1 = &conf;
	std::wstring szIniFile;
	TCHAR szValue[256];

	GetIniFile(szIniFile);

	FILE *f = nullptr;
	auto err = _wfopen_s(&f, szIniFile.c_str(), L"a+");
	if (!f) {
		MessageBoxW(NULL, L"Cannot save to ini!", L"USBqemu", MB_ICONERROR);
	} else
		fclose(f);

	swprintf_s(szValue,L"%u",Conf1->Log);
	WritePrivateProfileString(TEXT("Interface"), TEXT("Logging"),szValue,szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->DFPPass);
	WritePrivateProfileString(TEXT("Devices"), TEXT("DFP Passthrough"),szValue,szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->Port0);
	WritePrivateProfileString(TEXT("Devices"), TEXT("Port 0"),szValue,szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->Port1);
	WritePrivateProfileString(TEXT("Devices"), TEXT("Port 1"),szValue,szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->WheelType[0]);
	WritePrivateProfileString(TEXT("Devices"), TEXT("Wheel Type 1"),szValue,szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->WheelType[1]);
	WritePrivateProfileString(TEXT("Devices"), TEXT("Wheel Type 2"),szValue,szIniFile.c_str());

	WritePrivateProfileString(TEXT("Devices"), TEXT("USB Image"),Conf1->usb_img,szIniFile.c_str());

	WritePrivateProfileString(TEXT("Devices"), TEXT("Mic 1"), Conf1->mics[0].c_str(), szIniFile.c_str());
	WritePrivateProfileString(TEXT("Devices"), TEXT("Mic 2"), Conf1->mics[1].c_str(), szIniFile.c_str());

	swprintf_s(szValue, L"%" TEXT(SFMTs), Conf1->micApi.c_str());
	WritePrivateProfileString(TEXT("Devices"), TEXT("MicAPI"), szValue, szIniFile.c_str());

	swprintf_s(szValue,L"%u",Conf1->MicBuffering);
	WritePrivateProfileString(TEXT("Devices"), TEXT("Mic Buffering"),szValue,szIniFile.c_str());

	//WritePrivateProfileString("Joystick", "Player1", player_joys[0].c_str(), szIniFile);
	//WritePrivateProfileString("Joystick", "Player2", player_joys[1].c_str(), szIniFile);

}

void LoadConfig() {

	Config *Conf1 = &conf;
	std::wstring szIniFile;
	TCHAR szValue[MAX_PATH+1];

	GetIniFile(szIniFile);

	FILE *fp = nullptr;
	auto err = _wfopen_s(&fp, szIniFile.c_str(), L"rt");//check if ini really exists
	if (!fp)
	{
		memset(&conf, 0, sizeof(conf));
		conf.Log = 0;//default value
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);

	GetPrivateProfileString(TEXT("Interface"), TEXT("Logging"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->Log = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("DFP Passthrough"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->DFPPass = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("Port 0"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->Port0 = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("Port 1"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->Port1 = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("Wheel Type 1"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->WheelType[0] = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("Wheel Type 2"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->WheelType[1] = wcstoul(szValue, NULL, 10);

	GetPrivateProfileString(TEXT("Devices"), TEXT("USB Image"), NULL, Conf1->usb_img, sizeof(Conf1->usb_img), szIniFile.c_str());

	// Get mics
	TCHAR tmp[1024];
	GetPrivateProfileString(TEXT("Devices"), TEXT("Mic 1"), NULL, tmp, sizeof(tmp)/sizeof(*tmp), szIniFile.c_str());
	Conf1->mics[0] = tmp;

	GetPrivateProfileString(TEXT("Devices"), TEXT("Mic 2"), NULL, tmp, sizeof(tmp)/sizeof(*tmp), szIniFile.c_str());
	Conf1->mics[1] = tmp;

	GetPrivateProfileString(TEXT("Devices"), TEXT("MicAPI"), NULL, tmp, sizeof(tmp) / sizeof(*tmp), szIniFile.c_str());

#if UNICODE
	char tmpA[64] = { 0 };
	size_t num = 0;
	wcstombs_s(&num, tmpA, tmp, sizeof(tmpA));
	Conf1->micApi = tmpA;
#else
	Conf1->micApi = tmp;
#endif

	GetPrivateProfileString(TEXT("Devices"), TEXT("Mic Buffering"), NULL, szValue, 20, szIniFile.c_str());
	Conf1->MicBuffering = wcstoul(szValue, NULL, 10);
	if(Conf1->MicBuffering == 0)
		Conf1->MicBuffering = 50;

	//GetPrivateProfileString("Joystick", "Player1", NULL, szValue, MAX_PATH, szIniFile);
	//player_joys[0] = szValue;

	//GetPrivateProfileString("Joystick", "Player2", NULL, szValue, MAX_PATH, szIniFile);
	//player_joys[1] = szValue;
	
	return ;

}

