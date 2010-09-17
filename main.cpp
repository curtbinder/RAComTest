
#include <stdio.h>
#include <tchar.h>
#include <windows.h>


HANDLE g_hCom = NULL;
BYTE g_1S = 0x00;  // first byte to send
BYTE g_2S = 0x00;  // second byte to send
BYTE g_1R = 0x00;  // first byte to receive
BYTE g_2R = 0x00;  // second byte to receive

BOOL TestForRA();
BOOL OpenPort(int nCom);
BOOL TestComPort();
BOOL SendCommand();
BOOL ReadData();

int _tmain(int argc, _TCHAR* argv[])
{
    if ( ! TestForRA() )
    {
        // we failed to find RA Controller
        return 1;
    }
    return 0;
}

BOOL TestForRA()
{
    BOOL bRet = FALSE;
    int i;

    // TODO assume 20 ports for now, need to update this to be more efficient
    for ( i = 0; i < 20; i++ )
    {
        if ( g_hCom )
        {
            CloseHandle(g_hCom);
            g_hCom = NULL;
        }


        // open port, then close port
        if ( ! OpenPort(i) )
        {
            continue;
        }
        CloseHandle(g_hCom);
        g_hCom = NULL;
        if ( ! OpenPort(i) )
        {
            continue;
        }

        // We have a good com port, now let's see if it's a valid RA Controller
        if ( ! SendCommand() )
        {
            continue;
        }

        if ( ReadData() )
        {
            // we got the proper response
            bRet = true;
            break;
        }
    }  // for i

    // extra sanity check to make sure that the port is closed
    if ( g_hCom )
    {
        CloseHandle(g_hCom);
        g_hCom = NULL;
    }

    return bRet;
}

BOOL OpenPort(int nCom)
{
    TCHAR szPortName[32];
    _stprintf(szPortName, _T("COM%d"), nCom);
    g_hCom = CreateFile(szPortName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if ( (g_hCom == NULL) || (g_hCom == INVALID_HANDLE_VALUE) )
    {
        // port is not valid
        return FALSE;
    }
    if ( ! TestComPort() )
    {
        if ( g_hCom )
        {
            CloseHandle(g_hCom);
            g_hCom = NULL;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL TestComPort()
{
    DCB dcb;
    // prep the serial port for communication
    ZeroMemory(&dcb, sizeof(dcb));
    if ( ! GetCommState(g_hCom, &dcb) )
    {
        return FALSE;
    }

    // configure the com port values
    dcb.BaudRate	= CBR_19200;  // TODO maybe 57600, CBR_57600
    dcb.ByteSize	= 8;
    dcb.Parity	    = NOPARITY;
    dcb.StopBits	= ONESTOPBIT;

    if ( ! SetCommState(g_hCom, &dcb) )
    {
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
	DWORD dwBytesRead;
	if ( ReadFile(g_hCom,
                  &buf,
                  sizeof(buf),
                  &dwBytesRead,
                  0) )
	{
		// verify that the value read back is correct
		// should check to see if total bytes read are 2
		if ( (buf[0] == g_1R) && (buf[1] == g_2R) )
		{
		    return TRUE;
		}
	}
    return FALSE;
}
