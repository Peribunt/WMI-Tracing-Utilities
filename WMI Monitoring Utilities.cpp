#include "WMI Monitoring Utilities.h"

#define SafeRelease( x ) if ( x ) x->Release( )


UINT32                     g_IndicateCallbackCount;
VECTORED_INDICATE_CALLBACK g_IndicateCallbacks[ MAX_INDICATE_CALLBACKS ]{ };

IWbemLocator*  g_WbemLocator;
IWbemServices* g_WbemServices;

ULONG
WINAPI
EventSink::AddRef( 
	VOID 
	)
{
	return InterlockedIncrement( &m_lRef );
}

ULONG
WINAPI
EventSink::Release( 
	VOID 
	)
{
	ULONG ReferenceCount = InterlockedDecrement( &m_lRef );

	if ( ReferenceCount == NULL ) {
		//
		// Free the current instance from the heap if no more references exist
		//
		delete this;
	}

	return ReferenceCount;
}

HRESULT
WINAPI
EventSink::QueryInterface( 
	IN  REFIID  IID, 
	OUT LPVOID* Interface 
	)
{
	if ( IID == IID_IUnknown || IID == IID_IWbemObjectSink ) {

		*Interface = this;

		AddRef( );

		return WBEM_S_NO_ERROR;
	}

	return E_NOINTERFACE;
}

HRESULT
WINAPI
EventSink::Indicate( 
	IN LONG               ObjectCount, 
	IN IWbemClassObject** ObjectArray 
	)
{
	for ( UINT32 i = NULL; i < MAX_INDICATE_CALLBACKS; i++ ) 
	{
		if ( g_IndicateCallbacks[ i ].Callback != NULL &&
			 g_IndicateCallbacks[ i ].Owner    == this ) 
		{
			 g_IndicateCallbacks[ i ].Callback( ObjectCount, ObjectArray );
		}
	}

	return WBEM_S_NO_ERROR;
}

HRESULT
WINAPI
EventSink::SetStatus( 
	IN LONG    Flags, 
	IN HRESULT Result, 
	IN BSTR    StrParam, 
	IN IWbemClassObject* Object 
	)
{
	return WBEM_S_NO_ERROR;
}

HRESULT
WMIMU_Initialize( 
	VOID 
	)
{
	BOOL CoinitFailed = FALSE;
	
	//
	// Initialize COM
	//
	HRESULT Result = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if ( FAILED( Result ) ) {
		CoinitFailed = TRUE;
		goto BadResult;
	}

	//
	// Initialize COM security
	//
	Result = CoInitializeSecurity( 
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
		);

	if ( FAILED( Result ) ) {
		goto BadResult;
	}

	//
	// Create a Wbem Locator
	//
	Result = CoCreateInstance( 
		CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		( LPVOID* )&g_WbemLocator
		);

	if ( FAILED( Result ) ) {
		goto BadResult;
	}

	//
	// Connect to the WMI namespace
	//
	Result = g_WbemLocator->ConnectServer( 
		_bstr_t( "ROOT\\CIMV2" ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&g_WbemServices 
		);

	if ( FAILED( Result ) ) {
		goto BadResult;
	}

	Result = CoSetProxyBlanket(
		g_WbemServices,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
		);

	if ( FAILED( Result ) ) {
		goto BadResult;
	}

	return Result;

BadResult:

	SafeRelease( g_WbemServices );
	SafeRelease( g_WbemLocator );

	if ( CoinitFailed == TRUE )  {
		CoUninitialize( );
	}

	return Result;
}

BOOLEAN WMIMU_AddEventCallback_Spinlock = FALSE;
BOOL
WMIMU_AddEventCallback( 
	IN EventSink*        EventSink,
	IN INDICATE_CALLBACK Callback
	)
{
	BOOL Result = FALSE;

	//
	// Acquire a spinlock for safety
	//
	while ( InterlockedExchange8( ( CHAR* )&WMIMU_AddEventCallback_Spinlock, TRUE ) == TRUE ) {
		_mm_pause( );
	}

	//
	// If the count of callbacks is at it's limit, return early
	//
	if ( g_IndicateCallbackCount++ >= MAX_INDICATE_CALLBACKS ) {
		goto Return;
	}

	for ( UINT32 i = NULL; i < MAX_INDICATE_CALLBACKS; i++ )
	{
		//
		// Ignore entries that are populated
		//
		if ( g_IndicateCallbacks[ i ].Owner    != NULL &&
			 g_IndicateCallbacks[ i ].Callback != NULL ) 
		{ 
			continue;
		}

		//
		// Populate a free entry with the callback data
		//
		g_IndicateCallbacks[ i ].Owner    = EventSink;
		g_IndicateCallbacks[ i ].Callback = Callback;

		//
		// Increment the callback count
		//
		g_IndicateCallbackCount++;

		Result = TRUE;
		goto Return;
	}

Return:

	//
	// Release the spinlock
	//
	WMIMU_AddEventCallback_Spinlock = FALSE;

	return Result;
}

BOOLEAN WMIMU_RemoveEventCallback_Spinlock = FALSE;
BOOL
WMIMU_RemoveEventCallback(
	IN EventSink*        EventSink,
	IN INDICATE_CALLBACK Callback
	)
{
	BOOL Result = FALSE;

	//
	// Acquire a spinlock for safety
	//
	while ( InterlockedExchange8( ( CHAR* )&WMIMU_RemoveEventCallback_Spinlock, TRUE ) == TRUE ) {
		_mm_pause( );
	}

	for ( UINT32 i = NULL; i < MAX_INDICATE_CALLBACKS; i++ ) 
	{
		//
		// Invalidate the data the corresponds to the specified callback
		//
		if ( g_IndicateCallbacks[ i ].Callback == Callback ) 
		{
			RtlZeroMemory( &g_IndicateCallbacks[ i ], sizeof( VECTORED_INDICATE_CALLBACK ) );

			Result = TRUE;
		}
	}

	//
	// Release the spinlock
	//
	WMIMU_RemoveEventCallback_Spinlock = FALSE;

	return Result;
}

BOOLEAN WMIMU_RemoveEventMonitor_Spinlock = FALSE;
VOID
WMIMU_RemoveEventMonitor( 
	IN EventSink* EventSink
	)
{
	//
	// Acquire a spinlock for safety
	//
	while ( InterlockedExchange8( ( CHAR* )&WMIMU_RemoveEventMonitor_Spinlock, TRUE ) == TRUE ) {
		_mm_pause( );
	}

	for ( UINT32 i = NULL; i < MAX_INDICATE_CALLBACKS; i++ )
	{
		//
		// Invalidate all entries that correspond to the specified event sink
		//
		if ( g_IndicateCallbacks[ i ].Owner == EventSink ) {
			RtlZeroMemory( &g_IndicateCallbacks[ i ], sizeof( VECTORED_INDICATE_CALLBACK ) );
		}
	}

	//
	// Release the event sink
	//
	EventSink->Release( );

	//
	// Release the spinlock
	//
	WMIMU_RemoveEventMonitor_Spinlock = FALSE;
}

HRESULT
WMIMU_AddEventMonitor( 
	IN  LPCSTR      Query, 
	OUT EventSink** OutEventSink 
	)
{
	//
	// Allocate an event sink for this specific monitor
	//
	EventSink* Sink = new EventSink;

	if ( Sink == NULL || OutEventSink == NULL ) {
		return E_OUTOFMEMORY;
	}

	//
	// Output the newly created event sink
	//
	*OutEventSink = Sink;

	return g_WbemServices->ExecNotificationQueryAsync( 
		_bstr_t( "WQL" ), 
		_bstr_t( Query ),
		WBEM_FLAG_SEND_STATUS,
		NULL,
		Sink
		);
}

VOID
WMIMU_Uninitialize( 
	VOID 
	)
{
	g_WbemServices->Release( );
	g_WbemLocator->Release( );

	CoUninitialize( );
}
