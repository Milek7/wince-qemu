#include <windows.h>
#include <devload.h>

#define ZONE_INIT  DEBUGZONE(0)
#define ZONE_ERROR DEBUGZONE(1)
#define ZONE_WARN  DEBUGZONE(2)
#define ZONE_INFO  DEBUGZONE(3)

//------------------------------------------------------------------------------
//
// Function to open the driver key specified by the active key
//
// The caller is responsible for closing the returned HKEY
//
//------------------------------------------------------------------------------
HKEY
OpenDriverKey(
    const TCHAR* ActiveKey
    )
{
    TCHAR DevKey[256];
    HKEY hDevKey;
    HKEY hActive;
    DWORD ValType;
    DWORD ValLen;
    DWORD status;

    //
    // Get the device key from active device registry key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                ActiveKey,
                0,
                0,
                &hActive);
    if (status) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("pl011: OpenDriverKey RegOpenKeyEx(HLM\\%s) returned %d!!!\r\n"),
                  ActiveKey, status));
        return NULL;
    }

    hDevKey = NULL;

    ValLen = sizeof(DevKey);
    status = RegQueryValueEx(
                hActive,
                DEVLOAD_DEVKEY_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)DevKey,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("pl011: OpenDriverKey - RegQueryValueEx(%s) returned %d\r\n"),
                  DEVLOAD_DEVKEY_VALNAME, status));
        goto odk_fail;
    }

    //
    // Get the geometry values from the device key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                DevKey,
                0,
                0,
                &hDevKey);
    if (status) {
        hDevKey = NULL;
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("pl011: OpenDriverKey RegOpenKeyEx - DevKey(HLM\\%s) returned %d!!!\r\n"),
                  DevKey, status));
    }

odk_fail:
    RegCloseKey(hActive);
    return hDevKey;
}   // OpenDriverKey



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
RegGetValue(
    const WCHAR* ActiveKey,
    const WCHAR* pwsName,
    PDWORD pdwValue
    )
{
    HKEY DriverKey;
    DWORD ValType;
    DWORD status;
    DWORD dwBytes;

    DriverKey = OpenDriverKey(ActiveKey);
    if (!DriverKey) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("pl011: GetRegValue - Could not open registry key %s\r\n"), ActiveKey));
        return FALSE;
    }

    dwBytes = sizeof(DWORD);
    status = RegQueryValueEx(DriverKey, pwsName, NULL, &ValType, (PUCHAR)pdwValue, &dwBytes);

    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("pl011: No %s entry specified in the registry\r\n"), pwsName));
        RegCloseKey(DriverKey);
        return FALSE;
    }

    DEBUGMSG(ZONE_INIT,(TEXT("pl011: %s registry value = %d\r\n"), pwsName, *pdwValue));
    RegCloseKey(DriverKey);
    return TRUE;
}