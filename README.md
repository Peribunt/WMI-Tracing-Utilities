# WMI Tracing Utilities
Utilities for performing asynchronous WQL queries for specific events. For a list of events that can be traced. Please refer to the [WMI System Classes](https://learn.microsoft.com/en-us/windows/win32/wmisdk/wmi-system-classes) under [Windows Management Instrumentation Docs](https://learn.microsoft.com/en-us/windows/win32/wmisdk/wmi-start-page)

## Documentation
* [Windows Management Instrumentation Docs](https://learn.microsoft.com/en-us/windows/win32/wmisdk/wmi-start-page)
  For full documentation about WMI and the WMISDK
* [WMI System Classes](https://learn.microsoft.com/en-us/windows/win32/wmisdk/wmi-system-classes)
  For events that can be traced using WQL queries
* [WQL(SQL for WMI)](https://learn.microsoft.com/en-us/windows/win32/wmisdk/wql-sql-for-wmi)
  For WQL syntax information

## Custom API Documentation
A slightly more descriptive and user friendly documentation for the WMIMU API functions
```cpp
//
// This function must be called exactly once to initialize the WMI monitoring API
//
// Return Value:
//
//          * NT_SUCCESS If everything initialized successfully
//          * The appropriate status code if an error occured
//
HRESULT
WMIMU_Initialize(
  VOID
  );
```
```cpp
//
// This function must be called exactly once on shutdown of the WMI monitoring API
//
VOID
WMIMU_Uninitialize(
  VOID
  );
```
```cpp
//
// Perform an asynchronous WMI event query and create an EventSink for callbacks
//
// Return Value:
//
//         * NT_SUCCESS if the function succeeded
//         * An appropriate status code if the function failed
//
HRESULT
WMIMU_AddEventMonitor(
  //
  // [in]  (LPCSTR)          Query: The event query for which an EventSink object will be created
  //
  IN  LPCSTR      Query,
  //
  // [out] (EventSink**) EventSink: A location in which the pointer to the newly created EventSink object will be stored
  //
  OUT EventSink** EventSink
  );
```
```cpp
//
// Add a callback function associated with a specific EventSink object
// to allow vectored callbacks to be invoked for a specific event
//
// Return Value:
//
//         * TRUE if the callback was successfully added
//         * FALSE if there was no space in the vectored callback list for the callback
//
BOOL
WMIMU_AddEventCallback(
  //
  // [in] (EventSink*)       EventSink: The EventSink object that belongs to a desired event query
  //
  IN EventSink*        EventSink,
  //
  // [in] (INDICATE_CALLBACK) Callback: The callback function that will be added onto
  //                                    the vectored list of callbacks that will be invoked at the desired event
  //
  IN INDICATE_CALLBACK Callback
  );
```
```cpp
//
// Remove a callback function that is associated to a specified EventSink obejct
//
// Return Value:
//
//         * TRUE if the callback was removed from the vectored callback list
//         * FALSE if the callback did not exist in the first place
//
BOOL
WMIMU_RemoveEventCallback(
  //
  // [in] (EventSink*)       EventSink: The EventSink object that belongs to a desired event query
  //
  IN EventSink*        EventSink,
  //
  // [in] (INDICATE_CALLBACK) Callback: The callback function that will be removed from
  //                                    the vectored list of callbacks that will be invoked at the desired event
  //
  IN INDICATE_CALLBACK Callback
  );
```
## Examples
### Tracing process creation:
```cpp
EventSink* CreationEventSink;

VOID
ProcessCreationCallback(
  IN LONG               ObjectCount,
  IN IWbemClassObject** ObjectList
  )
{
  for ( UINT32 i = NULL; i < ObjectCount; i++ )
  {
    BSTR Information = ( BSTR )NULL;

    //
    // Acquire the object text of the current objects(Which in this case contains a very descriptive Win32_Process object)
    //
    ObjectList[ i ]->GetObjectText( NULL, &Information );
    
    printf( "%ls", Information );
  }  
}

LONG
main(
  VOID
  )
{
  //
  // Initialize the WMIMU API 
  //
  HRESULT Result = WMIMU_Initialize( );

  if ( SUCCEEDED( Result ) )
  {
    //
    // Create an EventSink for instance creation events in which the target instance is a process
    //
    Result = WMIMU_AddEventMonitor(
      "SELECT * FROM __InstanceCreationEvent WITHIN 1 WHERE TargetInstance ISA 'Win32_Process'",
      &CreationEventSink
      );

    if ( SUCCEEDED( Result ) == FALSE ) {
      return Result;
    }

    WMIMU_AddEventCallback( CreationEventSink, ProcessCreationCallback );
  }
}
```
