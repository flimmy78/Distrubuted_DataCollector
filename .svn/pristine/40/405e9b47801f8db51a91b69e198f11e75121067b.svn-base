// DataCollector.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"

#ifdef WIN32
#include "./Service/DCService.h"
#include "./Service/service_installer.h"


int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR sServiceName[MAX_PATH] = { 0 };
	TCHAR szServicePath[MAX_PATH] = { 0 };

	::GetModuleFileName(NULL, szServicePath, MAX_PATH);
	_splitpath_s(szServicePath, NULL, 0, NULL, 0, 
		sServiceName, MAX_PATH, NULL, 0);

	DCService service(sServiceName);

	if (argc > 1)
	{
		if (_tcsicmp(argv[1], _T("install")) == 0)
		{
			_tprintf(_T("Installing service\n"));
			if (!ServiceInstaller::Install(service))
			{
				_tprintf(_T("Couldn't install service: %d\n"), ::GetLastError());
				return -1;
			}

			_tprintf(_T("Service installed\n"));
			return 0;
		}

		if (_tcsicmp(argv[1], _T("uninstall")) == 0)
		{
			_tprintf(_T("Uninstalling service\n"));
			if (!ServiceInstaller::Uninstall(service))
			{
				_tprintf(_T("Couldn't uninstall service: %d\n"), ::GetLastError());
				return -1;
			}

			_tprintf(_T("Service uninstalled\n"));
			return 0;
		}

		if (_tcsicmp(argv[1], _T("console")) == 0)
		{
			service.Run(argv[1]);
			return 0;
		}

		_tprintf(_T("Invalid argument\n"));
		return -1;
	}
	else
	{
		service.Run();
	}
	return 0;
}
#endif //WIN32

