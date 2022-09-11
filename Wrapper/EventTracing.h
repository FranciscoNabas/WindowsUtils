#pragma once

#include "Unmanaged.h"
#include <tdh.h>
#include <evntrace.h>
#include <Wbemidl.h>

#define RELEASECOMOBJECT(obj) if (NULL != obj) { obj->Release(); }

namespace Unmanaged::WindowsEventTracing
{
	class EventDecoder
	{
	public:
		explicit EventDecoder(_In_opt_ LPCWSTR searchpath)
		{
			PTDH_CONTEXT p = tdhcontext;
			
			if (nullptr != searchpath)
			{
				p->ParameterValue = reinterpret_cast<UINT_PTR>(searchpath);
				p->ParameterType = TDH_CONTEXT_WPP_TMFSEARCHPATH;
				p->ParameterSize = 0;
				p += 1;
			}

			tdhcontexcount = static_cast<BYTE>(p - tdhcontext);
		}

		/*
		For testing purposes only.
		This functions were copied from MS example on how to consume event records.
		*/

		// Live, parse FILETIME to System.DateTime
		void PrintFileTime(FILETIME const& filetime)
		{
			SYSTEMTIME systemtime = { };
			FileTimeToSystemTime(&filetime, &systemtime);
			wprintf(L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
				systemtime.wYear,
				systemtime.wMonth,
				systemtime.wDay,
				systemtime.wHour,
				systemtime.wMinute,
				systemtime.wSecond,
				systemtime.wMilliseconds);
		}

		// Information will not be printed on the console by unmanaged code
		void PrintIndent()
		{
			wprintf(L"%*ls", indentlevel * 2, L"");
		}

		void PrintWppStringProperty(_In_z_ LPCWSTR szPropertyName)
		{
			PROPERTY_DATA_DESCRIPTOR pdd = { reinterpret_cast<UINT_PTR>(szPropertyName) };

			ULONG status;
			ULONG cb = 0;
			status = TdhGetPropertySize(
				pevent,
				tdhcontexcount,
				tdhcontexcount ? tdhcontext : nullptr,
				1,
				&pdd,
				&cb);
			if (status == ERROR_SUCCESS)
			{
				if (propbuffer.size() < cb / 2)
				{
					propbuffer.resize(cb / 2);
				}

				status = TdhGetProperty(
					pevent,
					tdhcontexcount,
					tdhcontexcount ? tdhcontext : nullptr,
					1,
					&pdd,
					cb,
					reinterpret_cast<BYTE*>(propbuffer.data()));
			}

			if (status != ERROR_SUCCESS)
			{
				wprintf(L"[TdhGetProperty(%ls) error %u]", szPropertyName, status);
			}
			else
			{
				// Print the FormattedString property data (nul-terminated
				// wchar_t string).
				wprintf(L"%ls", propbuffer.data());
			}
		}

		void PrintGuid(GUID const& g)
		{
			wprintf(L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
				g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2],
				g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
		}

		bool IsStringEvent() const
		{
			return (pevent->EventHeader.Flags & EVENT_HEADER_FLAG_STRING_ONLY) != 0;
		}

		/*
		Converts a TRACE_EVENT_INFO offset (e.g. TaskNameOffset) into a string.
		*/
		_Ret_z_ LPCWSTR TeiString(unsigned offset)
		{
			return reinterpret_cast<LPCWSTR>(tevtinfobuffer.data() + offset);
		}

		/*
		Prints out the values of properties from begin..end.
		Called by PrintEventRecord for the top-level properties.
		If there are structures, this will be called recursively for the child
		properties.
		*/
		void PrintProperties(unsigned propBegin, unsigned propEnd)
		{
			TRACE_EVENT_INFO const* const pTei =
				reinterpret_cast<TRACE_EVENT_INFO const*>(tevtinfobuffer.data());

			for (unsigned propIndex = propBegin; propIndex != propEnd; propIndex += 1)
			{
				EVENT_PROPERTY_INFO const& epi = pTei->EventPropertyInfoArray[propIndex];

				// If this property is a scalar integer, remember the value in case it
				// is needed for a subsequent property's length or count.
				if (0 == (epi.Flags & (PropertyStruct | PropertyParamCount)) &&
					epi.count == 1)
				{
					switch (epi.nonStructType.InType)
					{
					case TDH_INTYPE_INT8:
					case TDH_INTYPE_UINT8:
						if ((pbdataend - pbdata) >= 1)
						{
							integervalues[propIndex] = *pbdata;
						}
						break;
					case TDH_INTYPE_INT16:
					case TDH_INTYPE_UINT16:
						if ((pbdataend - pbdata) >= 2)
						{
							integervalues[propIndex] = *reinterpret_cast<UINT16 const UNALIGNED*>(pbdata);
						}
						break;
					case TDH_INTYPE_INT32:
					case TDH_INTYPE_UINT32:
					case TDH_INTYPE_HEXINT32:
						if ((pbdataend - pbdata) >= 4)
						{
							auto val = *reinterpret_cast<UINT32 const UNALIGNED*>(pbdata);
							integervalues[propIndex] = static_cast<USHORT>(val > 0xffffu ? 0xffffu : val);
						}
						break;
					}
				}

				PrintIndent();

				// Print the property's name.
				wprintf(L"%ls:", epi.NameOffset ? TeiString(epi.NameOffset) : L"(noname)");

				indentlevel += 1;

				// We recorded the values of all previous integer properties just
				// in case we need to determine the property length or count.
				USHORT const propLength =
					epi.nonStructType.OutType == TDH_OUTTYPE_IPV6 &&
					epi.nonStructType.InType == TDH_INTYPE_BINARY &&
					epi.length == 0 &&
					(epi.Flags & (PropertyParamLength | PropertyParamFixedLength)) == 0
					? 16 // special case for incorrectly-defined IPV6 addresses
					: (epi.Flags & PropertyParamLength)
					? integervalues[epi.lengthPropertyIndex] // Look up the value of a previous property
					: epi.length;
				USHORT const arrayCount =
					(epi.Flags & PropertyParamCount)
					? integervalues[epi.countPropertyIndex] // Look up the value of a previous property
					: epi.count;

				// Note that PropertyParamFixedCount is a new flag and is ignored
				// by many decoders. Without the PropertyParamFixedCount flag,
				// decoders will assume that a property is an array if it has
				// either a count parameter or a fixed count other than 1. The
				// PropertyParamFixedCount flag allows for fixed-count arrays with
				// one element to be propertly decoded as arrays.
				bool isArray =
					1 != arrayCount ||
					0 != (epi.Flags & (PropertyParamCount | PropertyParamFixedCount));
				if (isArray)
				{
					wprintf(L" Array[%u]\n", arrayCount);
				}

				PEVENT_MAP_INFO pMapInfo = nullptr;

				// Treat non-array properties as arrays with one element.
				for (unsigned arrayIndex = 0; arrayIndex != arrayCount; arrayIndex += 1)
				{
					if (isArray)
					{
						// Print a name for the array element.
						PrintIndent();
						wprintf(L"%ls[%lu]:",
							epi.NameOffset ? TeiString(epi.NameOffset) : L"(noname)",
							arrayIndex);
					}

					if (epi.Flags & PropertyStruct)
					{
						// If this property is a struct, recurse and print the child
						// properties.
						wprintf(L"\n");
						PrintProperties(
							epi.structType.StructStartIndex,
							epi.structType.StructStartIndex + epi.structType.NumOfStructMembers);
						continue;
					}

					// If the property has an associated map (i.e. an enumerated type),
					// try to look up the map data. (If this is an array, we only need
					// to do the lookup on the first iteration.)
					if (epi.nonStructType.MapNameOffset != 0 && arrayIndex == 0)
					{
						switch (epi.nonStructType.InType)
						{
						case TDH_INTYPE_UINT8:
						case TDH_INTYPE_UINT16:
						case TDH_INTYPE_UINT32:
						case TDH_INTYPE_HEXINT32:
							if (mapbuffer.size() == 0)
							{
								mapbuffer.resize(sizeof(EVENT_MAP_INFO));
							}

							for (;;)
							{
								ULONG cbBuffer = static_cast<ULONG>(mapbuffer.size());
								ULONG status = TdhGetEventMapInformation(
									pevent,
									const_cast<LPWSTR>(TeiString(epi.nonStructType.MapNameOffset)),
									reinterpret_cast<PEVENT_MAP_INFO>(mapbuffer.data()),
									&cbBuffer);

								if (status == ERROR_INSUFFICIENT_BUFFER &&
									mapbuffer.size() < cbBuffer)
								{
									mapbuffer.resize(cbBuffer);
									continue;
								}
								else if (status == ERROR_SUCCESS)
								{
									pMapInfo = reinterpret_cast<PEVENT_MAP_INFO>(mapbuffer.data());
								}

								break;
							}
							break;
						}
					}

					bool useMap = pMapInfo != nullptr;

					// Loop because we may need to retry the call to TdhFormatProperty.
					for (;;)
					{
						ULONG cbBuffer = static_cast<ULONG>(propbuffer.size() * 2);
						USHORT cbUsed = 0;
						ULONG status;

						if (0 == propLength &&
							epi.nonStructType.InType == TDH_INTYPE_NULL)
						{
							// TdhFormatProperty doesn't handle INTYPE_NULL.
							if (propbuffer.empty())
							{
								propbuffer.push_back(0);
							}
							propbuffer[0] = 0;
							status = ERROR_SUCCESS;
						}
						else if (
							0 == propLength &&
							0 != (epi.Flags & (PropertyParamLength | PropertyParamFixedLength)) &&
							(epi.nonStructType.InType == TDH_INTYPE_UNICODESTRING ||
								epi.nonStructType.InType == TDH_INTYPE_ANSISTRING))
						{
							// TdhFormatProperty doesn't handle zero-length counted strings.
							if (propbuffer.empty())
							{
								propbuffer.push_back(0);
							}
							propbuffer[0] = 0;
							status = ERROR_SUCCESS;
						}
						else
						{
							status = TdhFormatProperty(
								const_cast<TRACE_EVENT_INFO*>(pTei),
								useMap ? pMapInfo : nullptr,
								pointersize,
								epi.nonStructType.InType,
								static_cast<USHORT>(
									epi.nonStructType.OutType == TDH_OUTTYPE_NOPRINT
									? TDH_OUTTYPE_NULL
									: epi.nonStructType.OutType),
								propLength,
								static_cast<USHORT>(pbdataend - pbdata),
								const_cast<PBYTE>(pbdata),
								&cbBuffer,
								propbuffer.data(),
								&cbUsed);
						}

						if (status == ERROR_INSUFFICIENT_BUFFER &&
							propbuffer.size() < cbBuffer / 2)
						{
							// Try again with a bigger buffer.
							propbuffer.resize(cbBuffer / 2);
							continue;
						}
						else if (status == ERROR_EVT_INVALID_EVENT_DATA && useMap)
						{
							// If the value isn't in the map, TdhFormatProperty treats it
							// as an error instead of just putting the number in. We'll
							// try again with no map.
							useMap = false;
							continue;
						}
						else if (status != ERROR_SUCCESS)
						{
							wprintf(L" [ERROR:TdhFormatProperty:%lu]\n", status);
						}
						else
						{
							wprintf(L" %ls\n", propbuffer.data());
							pbdata += cbUsed;
						}

						break;
					}
				}

				indentlevel -= 1;
			}
		}

		// If it's WPP event
		void PrintWppEvent()
		{
			/*
			TDH supports a set of known properties for WPP events:
			- "Version": UINT32 (usually 0)
			- "TraceGuid": GUID
			- "GuidName": UNICODESTRING (module name)
			- "GuidTypeName": UNICODESTRING (source file name and line number)
			- "ThreadId": UINT32
			- "SystemTime": SYSTEMTIME
			- "UserTime": UINT32
			- "KernelTime": UINT32
			- "SequenceNum": UINT32
			- "ProcessId": UINT32
			- "CpuNumber": UINT32
			- "Indent": UINT32
			- "FlagsName": UNICODESTRING
			- "LevelName": UNICODESTRING
			- "FunctionName": UNICODESTRING
			- "ComponentName": UNICODESTRING
			- "SubComponentName": UNICODESTRING
			- "FormattedString": UNICODESTRING
			- "RawSystemTime": FILETIME
			- "ProviderGuid": GUID (usually 0)
			*/

			// Use TdhGetProperty to get the properties we need.
			wprintf(L" ");
			PrintWppStringProperty(L"GuidName"); // Module name (WPP's "CurrentDir" variable)
			wprintf(L" ");
			PrintWppStringProperty(L"GuidTypeName"); // Source code file name + line number
			wprintf(L" ");
			PrintWppStringProperty(L"FunctionName");
			wprintf(L"\n");
			PrintIndent();
			PrintWppStringProperty(L"FormattedString");
			wprintf(L"\n");
		}

		// If it's non-WPP event
		void PrintNonWppEvent()
		{
			ULONG status;
			ULONG cb;

			// Try to get event decoding information from TDH.
			cb = static_cast<ULONG>(tevtinfobuffer.size());
			status = TdhGetEventInformation(
				pevent,
				tdhcontexcount,
				tdhcontexcount ? tdhcontext : nullptr,
				reinterpret_cast<TRACE_EVENT_INFO*>(tevtinfobuffer.data()),
				&cb);
			if (status == ERROR_INSUFFICIENT_BUFFER)
			{
				tevtinfobuffer.resize(cb);
				status = TdhGetEventInformation(
					pevent,
					tdhcontexcount,
					tdhcontexcount ? tdhcontext : nullptr,
					reinterpret_cast<TRACE_EVENT_INFO*>(tevtinfobuffer.data()),
					&cb);
			}

			if (status != ERROR_SUCCESS)
			{
				// TdhGetEventInformation failed so there isn't a lot we can do.
				// The provider ID might be helpful in tracking down the right
				// manifest or TMF path.
				wprintf(L" ");
				PrintGuid(pevent->EventHeader.ProviderId);
				wprintf(L"\n");
			}
			else
			{
				// TDH found decoding information. Print some basic info about the event,
				// then format the event contents.

				TRACE_EVENT_INFO const* const pTei =
					reinterpret_cast<TRACE_EVENT_INFO const*>(tevtinfobuffer.data());

				if (pTei->ProviderNameOffset != 0)
				{
					// Event has a provider name -- show it.
					wprintf(L" %ls", TeiString(pTei->ProviderNameOffset));
				}
				else
				{
					// No provider name so print the provider ID.
					wprintf(L" ");
					PrintGuid(pevent->EventHeader.ProviderId);
				}

				// Show core important event properties - try to show some kind of "event name".
				if (pTei->DecodingSource == DecodingSourceWbem ||
					pTei->DecodingSource == DecodingSourceWPP)
				{
					// OpcodeName is usually the best "event name" property for WBEM/WPP events.
					if (pTei->OpcodeNameOffset != 0)
					{
						wprintf(L" %ls", TeiString(pTei->OpcodeNameOffset));
					}

					wprintf(L"\n");
				}
				else
				{
					if (pTei->EventNameOffset != 0)
					{
						// Event has an EventName, so print it.
						wprintf(L" %ls", TeiString(pTei->EventNameOffset));
					}
					else if (pTei->TaskNameOffset != 0)
					{
						// EventName is a recent addition, so not all events have it.
						// Many events use TaskName as an event identifier, so print it if present.
						wprintf(L" %ls", TeiString(pTei->TaskNameOffset));
					}

					wprintf(L"\n");

					// Show EventAttributes if available.
					if (pTei->EventAttributesOffset != 0)
					{
						PrintIndent();
						wprintf(L"EventAttributes: %ls\n", TeiString(pTei->EventAttributesOffset));
					}
				}

				if (IsStringEvent())
				{
					// The event was written using EventWriteString.
					// We'll handle it later.
				}
				else
				{
					// The event is a MOF, manifest, or TraceLogging event.

					// To help resolve PropertyParamCount and PropertyParamLength,
					// we will record the values of all integer properties as we
					// reach them. Before we start, clear out any old values and
					// resize the vector with room for the new values.
					integervalues.clear();
					integervalues.resize(pTei->PropertyCount);

					// Recursively print the event's properties.
					PrintProperties(0, pTei->TopLevelPropertyCount);
				}
			}

			if (IsStringEvent())
			{
				// The event was written using EventWriteString.
				// We can print it whether or not we have decoding information.
				LPCWSTR pchData = static_cast<LPCWSTR>(pevent->UserData);
				unsigned cchData = pevent->UserDataLength / 2;
				PrintIndent();

				// It's probably nul-terminated, but just in case, limit to cchData chars.
				wprintf(L"%.*ls\n", cchData, pchData);
			}
		}

		// Not necessary. We will decode only one type of event
		bool IsWppEvent() const
		{
			return (pevent->EventHeader.Flags & EVENT_HEADER_FLAG_TRACE_MESSAGE) != 0;
		}

		/*
		End of temporary functions
		*/

		void PrintEventRecord(PEVENT_RECORD pevtrecord)
		{
			indentlevel = 1;
			pevent = pevtrecord;
			pbdata = static_cast<BYTE const*>(pevent->UserData);
			pbdataend = pbdata + pevent->UserDataLength;
			pointersize =
				pevent->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER
				? 4
				: pevent->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER
				? 8
				: sizeof(void*);
			
			PrintFileTime(reinterpret_cast<FILETIME const&>(pevent->EventHeader.TimeStamp));

			if (IsWppEvent())
			{
				PrintWppEvent();
			}
			else
			{
				PrintNonWppEvent();
			}
		}

	private:
		TDH_CONTEXT tdhcontext[1];
		BYTE tdhcontexcount;
		BYTE pointersize;
		BYTE indentlevel;
		PEVENT_RECORD pevent;
		BYTE const* pbdata;
		BYTE const* pbdataend;
		std::vector<USHORT> integervalues;
		std::vector<BYTE> tevtinfobuffer;
		std::vector<BYTE> mapbuffer;
		std::vector<WCHAR> propbuffer;
	};

	class TraceHandles
	{
	public:

		~TraceHandles()
		{
			CloseHandles();
		}

		void CloseHandles()
		{
			while (!handles.empty())
			{
				CloseTrace(handles.back());
				handles.pop_back();
			}
		}

		ULONG OpenTraceW(
			_Inout_ PEVENT_TRACE_LOGFILEW pLogFile)
		{
			ULONG status;

			handles.reserve(handles.size() + 1);
			TRACEHANDLE handle = ::OpenTraceW(pLogFile);
			if (handle == INVALID_PROCESSTRACE_HANDLE)
			{
				status = GetLastError();
			}
			else
			{
				handles.push_back(handle);
				status = 0;
			}

			return status;
		}

		ULONG ProcessTrace(
			_In_opt_ LPFILETIME pStartTime,
			_In_opt_ LPFILETIME pEndTime)
		{
			return ::ProcessTrace(
				handles.data(),
				static_cast<ULONG>(handles.size()),
				pStartTime,
				pEndTime);
		}

	private:

		std::vector<TRACEHANDLE> handles;
	};

	class EventMofProcessor
	{
	public:

		typedef struct _PROPERTY_LIST
		{
			LPWSTR Name;
			LONG CimType;
			IWbemQualifierSet* pQualifiers;
		}PROPERTY_LIST;

		EventMofProcessor()
		{
			HRESULT hresult = CoCreateInstance(
				CLSID_WbemAdministrativeLocator,
				NULL,
				CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
				IID_IUnknown,
				(void**)&locator
			);

			if (SUCCEEDED(hresult))
			{
				hresult = locator->ConnectServer(L"root\\WMI", NULL, NULL, NULL, 0, NULL, NULL, &service);
				locator->Release();

				hresult = service->GetObject(L"Registry_TypeGroup1", WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &regclass, NULL);
				if (SUCCEEDED(hresult))
				{
					hresult = regclass->GetPropertyQualifierSet(L"InitialTime", &initialtimeqset);
					hresult = regclass->GetPropertyQualifierSet(L"Status", &statusqset);
					hresult = regclass->GetPropertyQualifierSet(L"Index", &indexqset);
					hresult = regclass->GetPropertyQualifierSet(L"KeyHandle", &keyhandleqset);
					hresult = regclass->GetPropertyQualifierSet(L"KeyName", &keynameqset);

					PROPERTY_LIST pproplist[5] = {
						{ L"InitialTime", CIM_SINT64, initialtimeqset },
						{ L"Status", CIM_UINT32, statusqset },
						{ L"Index", CIM_UINT32, indexqset },
						{ L"KeyHandle", CIM_UINT32, keyhandleqset },
						{ L"KeyName", CIM_STRING, keynameqset }
					};
				}
			}
		}

		~EventMofProcessor()
		{
			RELEASECOMOBJECT(initialtimeqset);
			RELEASECOMOBJECT(statusqset);
			RELEASECOMOBJECT(indexqset);
			RELEASECOMOBJECT(keyhandleqset);
			RELEASECOMOBJECT(keynameqset);
			RELEASECOMOBJECT(regclass);
			RELEASECOMOBJECT(service);
		}

	private:

		IWbemServices* service = NULL;
		IWbemLocator* locator = NULL;
		IWbemClassObject* regclass = NULL;
		IWbemQualifierSet* initialtimeqset = NULL;
		IWbemQualifierSet* statusqset = NULL;
		IWbemQualifierSet* indexqset = NULL;
		IWbemQualifierSet* keyhandleqset = NULL;
		IWbemQualifierSet* keynameqset = NULL;
		
	};

	
}