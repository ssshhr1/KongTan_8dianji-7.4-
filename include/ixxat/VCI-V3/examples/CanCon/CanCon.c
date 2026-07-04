/*****************************************************************************
 IXXAT Automation GmbH
******************************************************************************

 Project : CANCON.EXE
 File    : CANCON.C
 Summary : Demo application for the IXXAT VCI C-API.
             
 Date    : 2005-07-28

 Compiler: 

 Remarks : This demo demonstrates the following VCI features
           - adapter selection
           - controller initialisation 
           - creation of a message channel
           - transmission / reception of CAN messages
******************************************************************************
 all rights reserved
*****************************************************************************/


/*****************************************************************************
 * include files
 ****************************************************************************/

 
#include <vcinpl.h>
#include <vcinpldynl.h>

#include <stdio.h>

#ifdef _CVI_
#include <ansi_c.h>     /* Required for compilation within LabWindows/CVI */
#else
#include <process.h>
#include <conio.h>
#endif

/*****************************************************************************
 * global variables
 ****************************************************************************/

static HANDLE hDevice;       // device handle
static LONG   lCtrlNo;       // controller number
static HANDLE hCanCtl;       // controller handle 
static HANDLE hCanChn;       // channel handle
static LONG   lMustQuit = 0; // quit flag for the receive thread


/*************************************************************************
 * local functions
*************************************************************************/

void SelectDevice ( BOOL fUserSelect );
void InitSocket   ( UINT32 dwCanNo );
void FinalizeApp  ( void );
void DisplayError ( HRESULT hResult );

void TransmitData ( void );
DWORD WINAPI ReceiveThread( LPVOID lpParam );


/*****************************************************************************
 Function:
  main

 Description:
  Main entry point of the application.

 Arguments:
  none

 Results:
  none
*****************************************************************************/
void main (void)
{
  // a varialble to get the last pressed key
  BYTE    bChar   = 0;
  HRESULT hResult = S_OK;

  printf(" >>>> VCI - C-API Example V3.1 (DYNCALL)<<<<\n");

  // Load VCI V3 Library
  if(hResult == S_OK)
    hResult = LoadVciNplLib();
  
  // Map General VCI Functions
  if(hResult == S_OK)
    hResult = MapGeneralVciFunctions();
  
  // Map Device Manager Functions
  if(hResult == S_OK)
    hResult = MapDeviceManagerFunctions();
  
  // Map Device Functions
  if(hResult == S_OK)
    hResult = MapDeviceFunctions();
  
  // Map CAN Controller Functions
  if(hResult == S_OK)
    hResult = MapCANControllerFunctions();
  
  // Map CAN Channel Functions
  if(hResult == S_OK)
    hResult = MapCANMessageChannelFunctions();

  if(hResult != S_OK)
  {
    printf("Initialization failed with errorcode 0x%08X\n", hResult);
    return ;
  }

  printf("\n Initializes the CAN with 125 kBaud");
  printf("\n Shows all received messages");
  printf("\n 't' + <ENTER> -key send a message with ID 100H");
  printf("\n 'q' + <ENTER> -key quit the application\n\n");

  printf(" Select Adapter...\r");  
  SelectDevice( TRUE );
  printf(" Select Adapter.......... OK !\n\n");

  printf(" Initialize CAN...\r");
  InitSocket( lCtrlNo );
  printf(" Initialize CAN............ OK !\n\n");
  
  //
  // start the receive thread
  //
  CreateThread( NULL, 0, ReceiveThread, NULL, 0, NULL);

  //
  // wait for keyboard hit transmit  CAN-Messages cyclically
  //
  while( 1 )
  { 
    // wait for the user to press a key
    bChar = getchar();

    // when the key is 't' ort 'T' the send a CAN message
    if ( (bChar == 't') || (bChar == 'T') )
      TransmitData();  

    // when the key is 't' ort 'T' then end the program
    if ( (bChar == 'q') || (bChar == 'Q') )
      break;

    Sleep(1);
  } 

  //
  // tell receive thread to quit
  //
  InterlockedExchange(&lMustQuit, 1);

  printf(" Free VCI - Resources...\r");
  FinalizeApp();
  printf(" Free VCI - Resources........ OK !\n\n");

  // Unmap all Function
  UnmapGeneralVciFunctions();
  UnmapDeviceManagerFunctions();
  UnmapDeviceFunctions();
  UnmapCANControllerFunctions();
  UnmapCANMessageChannelFunctions();

  // Unload VCI Library
  FreeVciNplLib();
}

/*****************************************************************************
 Function:
  SelectDevice

 Description:
  Selects the first CAN adapter.

 Arguments:
  fUserSelect -> If this parameter is set to TRUE the functions display
                 a dialog box which allows the user to select the device.

 Results:
  none
*****************************************************************************/
void SelectDevice( BOOL fUserSelect )
{
  HRESULT hResult; // error code

  if (fUserSelect == FALSE)
  {
    HANDLE        hEnum;   // enumerator handle
    VCIDEVICEINFO sInfo;   // device info

    //
    // open the device list
    //
    hResult = DYNCALL(vciEnumDeviceOpen)(&hEnum);

    //
    // retrieve information about the first
    // device within the device list
    //
    if (hResult == VCI_OK)
    {
      hResult = DYNCALL(vciEnumDeviceNext)(hEnum, &sInfo);
    }

    //
    // close the device list (no longer needed)
    //
    DYNCALL(vciEnumDeviceClose)(hEnum);

    //
    // open the device
    //
    if (hResult == VCI_OK)
    {
      hResult = DYNCALL(vciDeviceOpen)(&sInfo.VciObjectId, &hDevice);
    }

    //
    // always select controller 0
    //
    lCtrlNo = 0;
  }
  else
  {
    //
    // open a device selected by the user
    //
    hResult = DYNCALL(vciDeviceOpenDlg)(NULL, &hDevice);
  }

  DisplayError(hResult);
}

/*************************************************************************
 Function: 
  InitSocket

 Description : 
  Opens the specified socket, creates a message channel, initializes
  and starts the CAN controller.

 Arguments: 
  dwCanNo -> Number of the CAN controller to open.

 Results:
  none

 Remarks:
  If <dwCanNo> is set to 0xFFFFFFFF, the function shows a dialog box
  which allows the user to select the VCI device and CAN controller.
*************************************************************************/
void InitSocket(UINT32 dwCanNo)
{
  HRESULT hResult;

  //
  // create a message channel
  //
  if (hDevice != NULL)
  {
    //
    // create and initialize a message channel
    //
    hResult = DYNCALL(canChannelOpen)(hDevice, dwCanNo, FALSE, &hCanChn);

    //
    // initialize the message channel
    //
    if (hResult == VCI_OK)
    {
      UINT16 wRxFifoSize  = 1024;
      UINT16 wRxThreshold = 1;
      UINT16 wTxFifoSize  = 128;
      UINT16 wTxThreshold = 1;

      hResult = DYNCALL(canChannelInitialize)( hCanChn,
                                               wRxFifoSize, wRxThreshold,
                                               wTxFifoSize, wTxThreshold);
    }

    //
    // activate the CAN channel
    //
    if (hResult == VCI_OK)
    {
      hResult = DYNCALL(canChannelActivate)(hCanChn, TRUE);
    }

    //
    // open the CAN controller
    //
    if (hResult == VCI_OK)
    {
      hResult = DYNCALL(canControlOpen)(hDevice, dwCanNo, &hCanCtl);
      // this function fails if the controller is in use
      // by another application.
    }

    //
    // initialize the CAN controller
    //
    if (hResult == VCI_OK)
    { 
      hResult = DYNCALL(canControlInitialize)( hCanCtl, CAN_OPMODE_STANDARD,
                                               CAN_BT0_125KB, CAN_BT1_125KB);
    }

    //
    // set the acceptance filter
    //
    if (hResult == VCI_OK)
    { 
       hResult = DYNCALL(canControlSetAccFilter)( hCanCtl, FALSE,
                                                  CAN_ACC_CODE_ALL, CAN_ACC_MASK_ALL);
    }

    //
    // start the CAN controller
    //
    if (hResult == VCI_OK)
    {
      hResult = DYNCALL(canControlStart)(hCanCtl, TRUE);
    }
  }
  else
  {
    hResult = VCI_E_INVHANDLE;
  }

  DisplayError(hResult);
}

/*************************************************************************
 Function: 
  TransmitData

 Description: 
  Transmits a CAN message with ID 0x100.

 Arguments: 
  none

 Results:
  none
*************************************************************************/
void TransmitData(void)
{
  HRESULT hResult;
  CANMSG  sCanMsg;
  UINT8   i;

  sCanMsg.dwTime   = 0;
  sCanMsg.dwMsgId  = 0x100;    // CAN message identifier

  sCanMsg.uMsgInfo.Bytes.bType  = CAN_MSGTYPE_DATA;
  sCanMsg.uMsgInfo.Bytes.bFlags = CAN_MAKE_MSGFLAGS(8,0,1,0,0);

  for (i = 0; i < sCanMsg.uMsgInfo.Bits.dlc; i++ )
  {
    sCanMsg.abData[i] = i;
  }

  // write the CAN message into the transmit FIFO
  hResult = DYNCALL(canChannelSendMessage)(hCanChn, INFINITE, &sCanMsg);
}

/*************************************************************************
 Function: 
  ReceiveThread

 Description: 
  Receive thread.

 Arguments: 
  none

 Results:
  none
*************************************************************************/
DWORD WINAPI ReceiveThread( LPVOID lpParam )
{
  HRESULT hResult;
  CANMSG  sCanMsg;
  
  while ( lMustQuit == 0 )
  {
    // read a CAN message from the receive FIFO

    hResult = DYNCALL(canChannelReadMessage)(hCanChn, 100, &sCanMsg);

    if (hResult == VCI_OK)
    {
      if (sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_DATA)
      {
        //
        // show data frames
        //
        if (sCanMsg.uMsgInfo.Bits.rtr == 0)
        {
          UINT8 j;

          printf("\nTime: %10u  ID: %3X  DLC: %1u  Data:",
                  sCanMsg.dwTime,
                  sCanMsg.dwMsgId,
                  sCanMsg.uMsgInfo.Bits.dlc);

          for (j = 0; j < sCanMsg.uMsgInfo.Bits.dlc; j++)
          {
            printf(" %.2X", sCanMsg.abData[j]);
          }
        }
        else
        {
          printf("\nTime: %10u ID: %3X  DLC: %1u  Remote Frame",
                 sCanMsg.dwTime,
                 sCanMsg.dwMsgId,
                 sCanMsg.uMsgInfo.Bits.dlc);
        }
      }
      else if (sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_INFO)
      {
        //
        // show informational frames
        //
        switch (sCanMsg.abData[0])
        {
          case CAN_INFO_START: printf("\nCAN started..."); break;
          case CAN_INFO_STOP : printf("\nCAN stoped...");  break;
          case CAN_INFO_RESET: printf("\nCAN reseted..."); break;
        }
      }
      else if (sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_ERROR)
      {
        //
        // show error frames
        //
        switch (sCanMsg.abData[0])
        {
          case CAN_ERROR_STUFF: printf("\nstuff error...");          break; 
          case CAN_ERROR_FORM : printf("\nform error...");           break; 
          case CAN_ERROR_ACK  : printf("\nacknowledgment error..."); break;
          case CAN_ERROR_BIT  : printf("\nbit error...");            break; 
          case CAN_ERROR_CRC  : printf("\nCRC error...");            break; 
          case CAN_ERROR_OTHER:
          default             : printf("\nother error...");          break;
        }
      }
    }
  }

  return 0;
}

/*************************************************************************
 Function: 
  FinalizeApp

 Description: 
  Finalizes the application

 Arguments: 
  none
  
 Results:
  none
*************************************************************************/
void FinalizeApp()
{
  //
  // close all open handles

  DYNCALL(canChannelClose)(hCanChn);
  DYNCALL(canControlClose)(hCanCtl);

  DYNCALL(vciDeviceClose)(hDevice);
}

/*************************************************************************
 Function: 
  DisplayError

 Description : 
  This function displays a message box for the specified error code.

 Arguments : 
  hResult -> Error code or -1 to display the error code returned
             by GetLastError().

 Results:
  none
*************************************************************************/
void DisplayError(HRESULT hResult)
{
  char szError[VCI_MAX_ERRSTRLEN];

  if (hResult != NO_ERROR)
  {
    if (hResult == -1)
      hResult = GetLastError();

    szError[0] = 0;
    DYNCALL(vciFormatError)(hResult, szError, sizeof(szError));
    MessageBox(NULL, szError, "VCI Demo", MB_OK | MB_ICONSTOP);
  }
}
