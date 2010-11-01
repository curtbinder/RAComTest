/*
 * Copyright 2010 / Curt Binder
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

TCHAR* g_sVersion = "1.05";
HANDLE g_hCom = NULL;
BYTE g_1S = 0x30;  // first byte to send
BYTE g_2S = 0x20;  // second byte to send
BYTE g_1R = 0x14;  // first byte to receive
BYTE g_2R = 0x10;  // second byte to receive
int g_iStartPort = 1;
int g_iStopPort = 99;
int g_iComTimeout = 5;
int g_iBaudRate = CBR_57600;

BOOL g_fVerbose = FALSE;
BOOL g_fEnumPorts = FALSE;
BOOL g_fListPorts = FALSE;
BOOL g_fForceAllPorts = FALSE;

BOOL TestForRA();
BOOL OpenPort(int nCom);
BOOL TestComPort();
BOOL SendCommand();
BOOL ReadData();
BOOL GetPorts(int ports[], int &count, BOOL bGetCount, BOOL bFillArray);
BOOL FillPortsArray(int ports[], int count);
void ListPorts();
int CountPorts();

void Verbose(const TCHAR* format, ...)
{
    if ( g_fVerbose )
    {
        va_list arglist;
        va_start(arglist, format);
        _vtprintf(format, arglist);
        va_end(arglist);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    int i;
    for ( i = 1; i < argc; i++ )
    {
        // loop through the parameters
        if ( _tcsicmp(argv[i], _T("verbose")) == 0 )
        {
            // enable verbose
            g_fVerbose = TRUE;
        }
        if ( _tcsnicmp(argv[i], _T("com="), 4) == 0 )
        {
            if ( g_fForceAllPorts )
            {
                _tprintf(_T("Cannot specify ALLPORTS and a specific port at the same time\n"));
                return 3;
            }
            if ( _tcsicmp(argv[i]+4, _T("")) == 0 )
            {
                _tprintf(_T("Invalid usage (%s). ex. COM=5"), argv[i]);
                return 3;
            }
            int x = _ttoi(argv[i]+4);
            if ( (x < 1) || (x > 99) )
            {
                _tprintf(_T("Invalid usage (%s). ex. COM=#, where # is between 1 and 99\n"), argv[i]);
                return 3;
            }
            g_iStartPort = x;
            g_iStopPort = x;
        }
        if ( _tcsnicmp(argv[i], _T("timeout="), 8) == 0 )
        {
            if ( _tcsicmp(argv[i]+8, _T("")) == 0 )
            {
                _tprintf(_T("Invalid usage (%s). ex. TIMEOUT=5\n"), argv[i]);
                return 5;
            }
            int x = _ttoi(argv[i]+8);
            if ( (x <= 0) || (x > 20) )
            {
                _tprintf(_T("Invalid usage (%s). ex. TIMEOUT=#, where # is between 1 and 20\n"), argv[i]);
                return 5;
            }
            g_iComTimeout = x;
        }
        if ( _tcsnicmp(argv[i], _T("baudrate="), 9) == 0 )
        {
            if ( _tcsicmp(argv[i]+9, _T("")) == 0 )
            {
                _tprintf(_T("Invalid usage (%s). ex. BAUDRATE=57600\n"), argv[i]);
                return 6;
            }
            unsigned long ulRate = _ttol(argv[i]+9);
            int x;
            switch ( ulRate )
            {
                case 9600:
                {
                    x = CBR_9600;
                    break;
                }
                case 19200:
                {
                    x = CBR_19200;
                    break;
                }
                default:
                case 57600:
                {
                    x = CBR_57600;
                    break;
                }
                case 115200:
                {
                    x = CBR_115200;
                    break;
                }
            }
            g_iBaudRate = x;
        }
        if ( (_tcsicmp(argv[i], _T("usage")) == 0) ||
             (_tcsicmp(argv[i], _T("help")) == 0) )
        {
            _tprintf(_T("Searches the COM ports on the system for the Reef Angel Controller.  If found,\n"));
            _tprintf(_T("returns 0.  Otherwise, a value > 0 is returned.  Does not display any output\n"));
            _tprintf(_T("unless the verbose option is specified.\n\n"));
            _tprintf(_T("Version:  %s\n"), g_sVersion);
            _tprintf(_T("Usage:  %s [OPTIONS]\n\n"), argv[0]);
            _tprintf(_T("Options:\n"));
            _tprintf(_T("    verbose     Enables displaying all messages\n"));
            _tprintf(_T("    com=#       Specifically tests specified COM port.  (# from 1 to 99)\n"));
            _tprintf(_T("    timeout=#   Sets the timeout for waiting for data back from the controller.\n"));
            _tprintf(_T("                (# from 1 to 20 seconds).  Default is 5 seconds.\n"));
            _tprintf(_T("    baudrate=#  Sets the baudrate for the COM port.  Default is 57600.\n"));
            _tprintf(_T("                Other values:   9600, 19200, 57600, 115200\n"));
            _tprintf(_T("    list        Lists the COM ports available on the system.\n"));
            _tprintf(_T("    allports    Forces all ports from 1 to 99 to be tested,\n"));
            _tprintf(_T("                instead of just the available ports on the system\n"));
            _tprintf(_T("    usage|help  Prints this screen\n"));
            _tprintf(_T("\n"));
            return 4;
        }
        if ( _tcsicmp(argv[i], _T("list")) == 0 )
        {
            ListPorts();
            return 4;
        }
        if ( _tcsicmp(argv[i], _T("allports")) == 0 )
        {
            if ( g_iStartPort == g_iStopPort )
            {
                _tprintf(_T("Cannot specify ALLPORTS and a specific port at the same time\n"));
                return 3;
            }
            g_fForceAllPorts = TRUE;
        }
    }

    if ( ! TestForRA() )
    {
        // we failed to find RA Controller
        Verbose(_T("Failed to find RA Controller\n"));
        return 1;
    }
    Verbose(_T("Found RA Controller\n"));
    return 0;
}

BOOL TestForRA()
{
    BOOL fRet = FALSE;
    int i, x;
    int * ports = NULL;
    int count = CountPorts();
    ports = new int[count];

    if ( ! FillPortsArray(ports, count) )
    {
        Verbose(_T("Failed to get list of COM ports\n"));
        return fRet;
    }

    for ( x = 0; x < count; x++ )
    {
        i = ports[x];
        Verbose(_T("Testing COM%d\n"), i);
        if ( g_hCom )
        {
            CloseHandle(g_hCom);
            g_hCom = NULL;
        }

        Verbose(_T("(COM%d): Opening port\n"), i);
        if ( ! OpenPort(i) )
        {
            Verbose(_T("(COM%d): Failed to open port\n"), i);
            continue;
        }
        if ( g_hCom )
        {
            CloseHandle(g_hCom);
            Verbose(_T("(COM%d): Closing port\n"), i);
            g_hCom = NULL;
        }
        Verbose(_T("(COM%d): Opening port (again)\n"), i);
        if ( ! OpenPort(i) )
        {
            Verbose(_T("(COM%d): Failed to open port (again)\n"), i);
            continue;
        }

        Sleep(1000);
        // We have a good com port, now let's see if it's a valid RA Controller
        Verbose(_T("(COM%d): Sending command (0x%02X, 0x%02X)\n"), i, g_1S, g_2S);
        if ( ! SendCommand() )
        {
            Verbose(_T("(COM%d Failed to send command\n"), i);
            continue;
        }

        Verbose(_T("(COM%d): Reading response, expecting (0x%02X, 0x%02X)\n"), i, g_1R, g_2R);
        if ( ReadData() )
        {
            // we got the proper response
            fRet = true;
            Verbose(_T("(COM%d): Valid response\n"), i);
            break;
        }
    }  // for i

    // extra sanity check to make sure that the port is closed
    if ( g_hCom )
    {
        CloseHandle(g_hCom);
        g_hCom = NULL;
    }

    if ( ports )
    {
        delete [] ports;
    }

    return fRet;
}

BOOL OpenPort(int nCom)
{
    TCHAR szPortName[32];
    _stprintf(szPortName, _T("\\\\.\\COM%d"), nCom);
    g_hCom = CreateFile(szPortName,
                        GENERIC_WRITE | GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if ( (g_hCom == NULL) || (g_hCom == INVALID_HANDLE_VALUE) )
    {
        // port is not valid
        Verbose(_T("(COM%d): Invalid COM port\n"), nCom);
        return FALSE;
    }

    Verbose(_T("(COM%d): Testing COM state\n"), nCom);
    if ( ! TestComPort() )
    {
        if ( g_hCom )
        {
            CloseHandle(g_hCom);
            g_hCom = NULL;
        }
        Verbose(_T("(COM%d): Failed to test COM port\n"), nCom);
        return FALSE;
    }
    Verbose(_T("(COM%d): Opened\n"), nCom);
    return TRUE;
}

BOOL TestComPort()
{
    DCB dcb;
    // prep the serial port for communication
    ZeroMemory(&dcb, sizeof(dcb));
    Verbose(_T("       - Getting COM state\n"));
    if ( ! GetCommState(g_hCom, &dcb) )
    {
        Verbose(_T("       - Failed getting COM state\n"));
        return FALSE;
    }

    // configure the com port values
    dcb.BaudRate	= g_iBaudRate;
    dcb.ByteSize	= 8;
    dcb.Parity	    = NOPARITY;
    dcb.StopBits	= ONESTOPBIT;

    Verbose(_T("       - Updating COM state\n"));
    if ( ! SetCommState(g_hCom, &dcb) )
    {
        Verbose(_T("       - Failed setting COM state\n"));
        return FALSE;
    }

    COMMTIMEOUTS ct;
    ZeroMemory(&ct, sizeof(ct));
    ct.ReadIntervalTimeout = MAXDWORD;
    ct.ReadTotalTimeoutMultiplier = MAXDWORD;
    ct.ReadTotalTimeoutConstant = g_iComTimeout * 1000;
    Verbose(_T("       - Setting %d second COM timeout\n"), g_iComTimeout);
    if ( ! SetCommTimeouts(g_hCom, &ct) )
    {
        Verbose(_T("       - Failed setting COM timeout\n"));
        return FALSE;
    }

    // valid com port
    return TRUE;
}

BOOL SendCommand()
{
	BYTE buf[2];
	buf[0] = g_1S;
	buf[1] = g_2S;
	DWORD dwBytesWritten;
	if ( ! WriteFile(g_hCom,
					 buf,
					 sizeof(buf),
					 &dwBytesWritten,
					 0) )
	{
		return FALSE;
	}
    return TRUE;
}

BOOL ReadData()
{
	BYTE buf[2] = {0, 0};
	BYTE b;
	DWORD dwBytesRead;
	DWORD dwTotalBytes = 0;
	BOOL fDone = FALSE;
    int pos = 0;
    BOOL fRet = FALSE;

    do
    {
        if ( ReadFile(g_hCom, &b, sizeof(b), &dwBytesRead, NULL) )
        {
            if ( dwBytesRead > 0 )
            {
                dwTotalBytes += dwBytesRead;
                Verbose(_T("       - Read (0x%02X)\n"), b);
                buf[pos] = b;
                pos++;
            }
            else if ( dwBytesRead == 0 )
            {
                // EOF
                Verbose(_T("       - Read NULL\n"));
                fDone = TRUE;
            }
        }
        else
        {
            Verbose(_T("       - Error Reading (%ld)\n"), GetLastError());
            fDone = TRUE;
        }
        if ( dwTotalBytes == 2 )
        {
            fDone = TRUE;
        }
    } while ( ! fDone );

    if ( (buf[0] == g_1R) && (buf[1] == g_2R) )
    {
        // valid RA response
        fRet = TRUE;
    }
    return fRet;
}

BOOL GetPorts(int ports[], int &count, BOOL bGetCount, BOOL bFillArray)
{
    BOOL fRet = FALSE;
    TCHAR buf[65535];
    unsigned long dwChars = QueryDosDevice(NULL, buf, sizeof(buf));
    int x = 0;

    if ( dwChars == 0 )
    {
        Verbose(_T("Error with querydosdevice:  (%ld)\n"), GetLastError());
        fRet = FALSE;
    }
    else
    {
        if ( g_fListPorts )
        {
            printf("Available COM ports\n");
            printf("-------------------\n");
        }

        TCHAR *ptr = buf;
        while (dwChars)
        {
            int port;
            if ( sscanf(ptr, "COM%d", &port) == 1 )
            {
                if ( g_fListPorts )
                {
                    printf("%s\n", ptr);
                }
                else
                {
                    // add to list of com ports
                    // store the port number in the array
                    if ( bFillArray )
                    {
                        ports[x] = port;
                    }
                }
                x++;
            }
            TCHAR *temp_ptr = strchr(ptr,0);
            dwChars -= (DWORD)((temp_ptr-ptr)/sizeof(TCHAR)+1);
            ptr = temp_ptr+1;
        }
        fRet = TRUE;

        if ( bGetCount )
        {
            count = x;
        }
        if ( g_fListPorts )
        {
            printf("-------------------\n");
            printf("Total:  %d port%c\n", x, (x>1)?'s':' ');
        }
    }

    return fRet;
}

BOOL FillPortsArray(int ports[], int count)
{
    BOOL fRet = FALSE;
    if ( g_fForceAllPorts )
    {
        for ( int i = 0; i < count; i++ )
        {
            ports[i] = i+1;
        }
        fRet = TRUE;
    }
    else
    {
        if ( count == 1 )
        {
            ports[0] = g_iStartPort;
            fRet = TRUE;
        }
        else
        {
            fRet = GetPorts(ports, count, FALSE, TRUE);
        }
    }
    return fRet;
}

void ListPorts()
{
    g_fListPorts = TRUE;
    int count = 0;
    GetPorts(NULL, count, FALSE, FALSE);
}

int CountPorts()
{
    int count = 0;
    if ( g_fForceAllPorts )
    {
        count = 99;
    }
    else
    {
        if ( g_iStartPort == g_iStopPort )
        {
            // only testing 1 port based on command line input
            count = 1;
        }
        else
        {
            GetPorts(NULL, count, TRUE, FALSE);
        }
    }
    return count;
}
