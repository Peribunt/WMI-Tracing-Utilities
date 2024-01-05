#ifndef __WMI_MONITORING_UTILITIES_H__
#define __WMI_MONITORING_UTILITIES_H__

/**
* WMI Monitoring Utilities
* By ReaP
* 
* These utilities consist of wrapper functions and simplified APIs to recieve WMI notifications
*/

#include <wbemidl.h>
#include <comutil.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")

//
// The maximum allowed amount of callbacks allowed to be used in a single event sink
//
#define MAX_INDICATE_CALLBACKS 256

typedef VOID( WINAPI* INDICATE_CALLBACK )(
	IN LONG               ObjectCount,
	IN IWbemClassObject** ObjectArray
	);

typedef struct VECTORED_INDICATE_CALLBACK
{
	INDICATE_CALLBACK Callback;
	LPVOID            Owner;
};

extern UINT32                     g_IndicateCallbackCount;
extern VECTORED_INDICATE_CALLBACK g_IndicateCallbacks[ MAX_INDICATE_CALLBACKS ];

class EventSink : public IWbemObjectSink
{
	UINT32                     m_lRef;
	BOOLEAN                    bDone;

public:

	EventSink( ) : m_lRef( NULL ), bDone( NULL )//, m_IndicateCallbackCount( NULL ), m_IndicateCallbacks{ NULL }
	{
	}

	~EventSink( )
	{
		bDone = TRUE;
	}

	/**
	 * @brief Increment the reference counter for heap safety
	 * 
	 * @return The reference counter
	*/
	virtual
	ULONG
	WINAPI
	AddRef( 
		VOID
		);

	/**
	 * @brief Release the current instance, if the reference counter is zero. The current instance will be deallocated
	 * 
	 * @return The reference counter
	*/
	virtual
	ULONG
	WINAPI
	Release( 
		VOID 
		);

	/**
	 * @brief Placeholder function for the QueryInterface routine. There is no point in using this for any purpose
	 * 
	 * @param [in]       RIID: The IID of the desired interface
	 * @param [out] Interface: A pointer to the current instance if desired
	 * 
	 * @return WBEM_S_NO_ERROR if the current interface was supplied and desired
	 * @return E_NOINTERFACE if the EventSink was not the desired interface
	*/
	virtual
	HRESULT
	WINAPI
	QueryInterface( 
		IN  REFIID  RIID, 
		OUT LPVOID* Interface 
		);

	/**
	 * @brief Callback function that will be invoked if the criteria of a WQL query were met for the desired WMI notification
	 *        of the current event sink
	 * 
	 * @param [in] ObjectCount: The amount of objects
	 * @param [in] ObjectArray: The array of objects, the type of these objects can be narrowed down using the IWbemClassObject functions
	 * 
	 * @return 
	*/
	virtual
	HRESULT
	WINAPI
	Indicate( 
		IN LONG               ObjectCount, 
		IN IWbemClassObject** ObjectArray 
		);

	/**
	 * @brief Callback that will be invoked to return the status of a specific object
	 * 
	 * @param [in] Flags 
	 * @param [in] Result 
	 * @param [in] StrParam 
	 * @param [in] ObjParam 
	 * 
	 * @return 
	*/
	virtual
	HRESULT
	WINAPI
	SetStatus( 
		IN LONG              Flags, 
		IN HRESULT           Result, 
		IN BSTR              StrParam, 
		IN IWbemClassObject* ObjParam 
		);
};

/**
 * @brief Initialize the WMI monitoring utilities to make notification queries
 * 
 * @return NT_SUCCESS if the function succeeds
 * @return An error status if an error occured
*/
HRESULT
WMIMU_Initialize( 
	VOID 
	);

/**
 * @brief Uninitialize the WMI monitoring utilities
*/
VOID
WMIMU_Uninitialize( 
	VOID 
	);

/**
 * @brief Adds a callback for a specific event
 * 
 * @param [in] EventSink: The event sink that belongs to the desired event
 * @param [in]  Callback: The callback that will be called when EventSink::Indicate is called 
 *                        for this instance
 * 
 * @return TRUE if the function succeeds
 * @return FALSE if the maximum amount of callbacks has been reached
*/
BOOL
WMIMU_AddEventCallback( 
	IN EventSink*        EventSink,
	IN INDICATE_CALLBACK Callback
	);

/**
 * @brief Removes a callback for a specific event
 * 
 * @param [in] EventSink: The event sink that belongs to the desired event
 * @param [in]  Callback: The callback that will be called when EventSink::Indicate is called
 *                        for this instance
 * 
 * @return TRUE if the function succeeds
 * @return FALSE if the callback was not found to begin with
*/
BOOL
WMIMU_RemoveEventCallback( 
	IN EventSink*        EventSink,
	IN INDICATE_CALLBACK Callback
	);

/**
 * @brief Removes a previously added event monitor
 * 
 * @param [in] EventSink: The event sink that belongs to the specified event monitor
*/
VOID
WMIMU_RemoveEventMonitor( 
	IN EventSink*        EventSink
	);

/**
 * @brief Perform an asynchronous query using WQL to receive notifications on a specific event
 * 
 * @param [in]      Query: The event query for which the event sink will be made
 * @param [out] EventSink: The event sink
 * 
 * @return NT_SUCCESS if the function succeeds
 * @return An error code if an error occured
*/
HRESULT
WMIMU_AddEventMonitor(
	IN  LPCSTR      Query,
	OUT EventSink** EventSink
	);

#endif
