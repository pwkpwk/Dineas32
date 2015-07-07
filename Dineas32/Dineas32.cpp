// Dineas32.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

static DNS_QUERY_RESULT result = { 0 };
static DNS_QUERY_CANCEL cancel = { 0 };

static void PrintResult(_In_ const DNS_QUERY_RESULT *result)
{
	for (PDNS_RECORD record = result->pQueryRecords; record; record = record->pNext)
	{
		switch (record->wType)
		{
		case DNS_TYPE_TEXT:
			for (DWORD i = 0; i < record->Data.TXT.dwStringCount; ++i)
			{
				_putws(record->Data.TXT.pStringArray[i]);
			}
			break;
		}
	}
}

static VOID WINAPI DnsQueryCompletion( _In_ PVOID pQueryContext, _Inout_ PDNS_QUERY_RESULT pQueryResults )
{
	switch (pQueryResults->QueryStatus)
	{
	case ERROR_SUCCESS:
		::PrintResult(pQueryResults);
		::DnsRecordListFree(pQueryResults->pQueryRecords, DnsFreeRecordList);
		break;

	default:
		break;
	}

	::SetEvent(reinterpret_cast<HANDLE>(pQueryContext));
}

int _tmain(int argc, _TCHAR* argv[])
{
	DNS_QUERY_REQUEST request = { 0 };

	request.Version = DNS_QUERY_REQUEST_VERSION1;
	request.QueryName = L"_msradc.microsoft.com";
	request.QueryType = DNS_TYPE_TEXT;
	request.QueryOptions = DNS_QUERY_STANDARD;
	request.pDnsServerList = nullptr;
	request.InterfaceIndex = 0;
	request.pQueryCompletionCallback = DnsQueryCompletion;

	result.Version = DNS_QUERY_REQUEST_VERSION1;

	HANDLE completed = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	if (completed)
	{
		request.pQueryContext = completed;

		IP_ADAPTER_ADDRESSES *pAddresses = nullptr;
		ULONG bufLength = 0;
		DWORD rc;

		do
		{
			rc = ::GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufLength);

			if (ERROR_BUFFER_OVERFLOW == rc)
			{
				if (pAddresses)
					::LocalFree(pAddresses);
				pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(::LocalAlloc(LPTR, bufLength));
			}
		} while (ERROR_BUFFER_OVERFLOW == rc);

		if (ERROR_SUCCESS == rc)
		{
			DNS_STATUS status = ::DnsQueryEx(&request, &result, &cancel);

			switch (status)
			{
			case ERROR_SUCCESS:
				::PrintResult(&result);
				break;

			case DNS_REQUEST_PENDING:
				if (WAIT_OBJECT_0 != ::WaitForSingleObject(completed, 5000))
				{
					if (ERROR_SUCCESS == ::DnsCancelQuery(&cancel))
						::WaitForSingleObject(completed, INFINITE);
				}
				break;

			case DNS_INFO_NO_RECORDS:
				break;

			case ERROR_INVALID_PARAMETER:
				break;

			default:
				break;
			}

			::LocalFree(pAddresses);
		}

		::CloseHandle(completed);
	}

	return 0;
}

