#include "xlDAIOdemoDlg.h"

#include <algorithm>
#include <string>

const unsigned int messageCount{100};

// global variables needed, because we have free function "ReceiveThread", that uses them
bool gChannelActivated{false};  // our config is valid and channel is activated
bool gRXThreadRun{false};

unsigned int                gLastDigital{0u};  // last value of digital inputs
std::array<unsigned int, 4> gLastAnalog;     // last value of analog inputs @@@@

XLportHandle gPort; // global port handles 
HANDLE gRxHandle = nullptr;

unsigned long       gThreadID;
HANDLE              gThreadHandle = nullptr;


// background task receiving messages
DWORD WINAPI ReceiveThread (void *param) {
  UNREFERENCED_PARAMETER(param);
  unsigned int msgscnt, ret;
  XLevent msgs[messageCount];

  gRXThreadRun = true;

  while (gRXThreadRun) {
    if (!gChannelActivated) {
      Sleep(100); // suspend thread when measurement is not running
      continue;
    }

    if (gRxHandle) WaitForSingleObject(gRxHandle, 1); else Sleep(1);

    msgscnt = messageCount;
    if (xlReceive(gPort, &msgscnt, msgs) != XL_SUCCESS) continue;

    for (ret=0;ret<msgscnt;ret++){
      if (msgs[ret].tag == XL_RECEIVE_DAIO_DATA) {
        gLastDigital    = msgs[ret].tagData.daioData.value_digital;
        gLastAnalog[2]  = msgs[ret].tagData.daioData.value_analog[2];
        gLastAnalog[3]  = msgs[ret].tagData.daioData.value_analog[3];
      }
    }
  } // while
  return 0;
}

//DialogBox
CXlDAIOdemoDlg::CXlDAIOdemoDlg(CWnd* pParent)	: CDialog(CXlDAIOdemoDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CXlDAIOdemoDlg)
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CXlDAIOdemoDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXlDAIOdemoDlg)
  DDX_Control(pDX, IDC_EDIT4, mAnalogCtrls[3]);
  DDX_Control(pDX, IDC_EDIT3, mAnalogCtrls[2]);
  DDX_Control(pDX, IDC_EDIT2, mAnalogCtrls[1]);
  DDX_Control(pDX, IDC_EDIT1, mAnalogCtrls[0]);
	DDX_Control(pDX, IDC_LIST2, m_Screen);
	DDX_Control(pDX, IDC_SLIDER1, m_PwmValueCtrl);
	DDX_Control(pDX, IDC_CHECK4, mDigitalOuts[3]);
  DDX_Control(pDX, IDC_CHECK3, mDigitalOuts[2]);
  DDX_Control(pDX, IDC_CHECK2, mDigitalOuts[1]);
  DDX_Control(pDX, IDC_CHECK1, mDigitalOuts[0]);
	DDX_Control(pDX, IDC_LIST1, m_DigitalStateList);
	DDX_Control(pDX, IDC_BUTTON2, m_StopCtrl);
	DDX_Control(pDX, IDC_BUTTON1, m_StartCtrl);
	DDX_Text(pDX, IDC_EDIT1, mAnalogs[0]);
  DDX_Text(pDX, IDC_EDIT2, mAnalogs[1]);
  DDX_Text(pDX, IDC_EDIT3, mAnalogs[2]);
  DDX_Text(pDX, IDC_EDIT4, mAnalogs[3]);
	DDX_CBIndex(pDX, IDC_COMBO1, m_MeasurementFrequency);
	DDX_Slider(pDX, IDC_SLIDER1, m_PwmValue);
	DDX_CBIndex(pDX, IDC_COMBO2, m_PwmFrequency);
	DDX_Text(pDX, IDC_SELECTEDCHANNELNAME, m_SelectedChannelName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CXlDAIOdemoDlg, CDialog)
	//{{AFX_MSG_MAP(CXlDAIOdemoDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnStart)
	ON_BN_CLICKED(IDC_BUTTON2, OnStop)
	ON_BN_CLICKED(IDC_BUTTON3, OnSetAnalog)
	ON_BN_CLICKED(IDC_CHECK1, OnSetDigital1)
	ON_BN_CLICKED(IDC_CHECK2, OnSetDigital2)
	ON_BN_CLICKED(IDC_CHECK3, OnSetDigital3)
	ON_BN_CLICKED(IDC_CHECK4, OnSetDigital4)
	ON_BN_CLICKED(IDC_BUTTON4, OnMeasureNow)
	ON_BN_CLICKED(IDC_BUTTON5, OnSetConfiguration)
	ON_WM_TIMER()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER1, OnReleasedcaptureSlider1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Message Handler
BOOL CXlDAIOdemoDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, true);
	SetIcon(m_hIcon, false);

  // Start background task, and make sure it is running
  gThreadHandle = CreateThread(nullptr, 0, ReceiveThread, 0, 0, &gThreadID);
  if (gThreadHandle == nullptr) {
    MessageBox("Error: Cannot start receive thread.");
    exit(0);
  }
  // we will set priority to low, to avoid problems with UI when lot of messages are received
  SetThreadPriority(gThreadHandle, THREAD_PRIORITY_BELOW_NORMAL);
	
  // then we have to setup all dialog controls
  m_PwmValueCtrl.SetRange (0, 10000, true); // PWM is in percent value with two decimal places precision
  m_PwmValueCtrl.SetTicFreq (1000);

  SetTimer(0, 250, nullptr);                // default refresh timer
  m_StopCtrl.EnableWindow(false);           // channel is not activated

	return true;
}

// OnPaint
void CXlDAIOdemoDlg::OnPaint() {
	if (IsIconic())	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}	else	{
		CDialog::OnPaint();
	}
}

HCURSOR CXlDAIOdemoDlg::OnQueryDragIcon() {
	return (HCURSOR) m_hIcon;
}

// close application correctly
void CXlDAIOdemoDlg::OnCancel() {
  OnStop();
  if (gThreadHandle && TerminateProcess(gThreadHandle, 0)) {
    CloseHandle( gThreadHandle );
  }	
	CDialog::OnCancel();
}

// add message to output window and scroll it
void CXlDAIOdemoDlg::AddMessage(CString msg) {
  m_Screen.InsertString(-1,msg);
  m_Screen.SetTopIndex(m_Screen.GetCount( )-1);
}

// on periodic timer (default 500ms we will update measured values)
void CXlDAIOdemoDlg::OnTimer(UINT nIDEvent) {
  UNREFERENCED_PARAMETER(nIDEvent);
  unsigned int pos;
  CString      txt;
  if (!gChannelActivated) return;

  if (m_DigitalStateList.GetCount() != 8) { // @@@@ // we have wrong content in our output list
    m_DigitalStateList.ResetContent();
    for (pos = 0; pos < 8; pos++) {
      txt.Format ("D %d\tNo data", pos );
      m_DigitalStateList.InsertString(pos, txt);
    }
  }

  // update digital input values
  for (pos = 0; pos < 8; pos++) {
    txt.Format ("D %d\t%s", pos, ((1 << pos) & gLastDigital)? "High":"Low" );
    m_DigitalStateList.DeleteString(pos);
    m_DigitalStateList.InsertString(pos, txt);
  }

  txt.Format("%d", gLastAnalog [2]);
  mAnalogCtrls[2].SetWindowText(txt);
  txt.Format("%d", gLastAnalog [3]);
  mAnalogCtrls[3].SetWindowText(txt);  
}

// detect, if there is valid configuration - if not try to update
// returns false when was already defined
bool CXlDAIOdemoDlg::OnFirstStart() {
  unsigned int hwType, hwIndex, hwChannel, pos;
  XLdriverConfig cfg;

  // could we found valid configuration?
  if (xlGetApplConfig ("xlDAIOdemo", 0, &hwType, &hwIndex, &hwChannel, XL_BUS_TYPE_DAIO) == XL_SUCCESS) return FALSE;

  hwType    = 0;
  hwIndex   = 0;
  hwChannel = 0;

  // there was not valid configuration, so we will check present hardware
  // and use first hardware channel capable to DAIObus or first hardware channel with IOcab inserted

  if (xlGetDriverConfig (&cfg) != XL_SUCCESS) return FALSE;

  for (pos = 0; pos < cfg.channelCount; pos++) {

    // if we have no hardware yet - get first capable
    if (!hwType && (cfg.channel[pos].channelBusCapabilities & XL_BUS_COMPATIBLE_DAIO)) {
      hwType    = cfg.channel[pos].hwType;
      hwIndex   = cfg.channel[pos].hwIndex;
      hwChannel = cfg.channel[pos].hwChannel;
    }

    // if there is IOcab inserted, than take this one and leave
    if (cfg.channel[pos].channelBusCapabilities & XL_BUS_ACTIVE_CAP_DAIO) {
      hwType    = cfg.channel[pos].hwType;
      hwIndex   = cfg.channel[pos].hwIndex;
      hwChannel = cfg.channel[pos].hwChannel;
      xlSetApplConfig ("xlDAIOdemo", 0, hwType, hwIndex, hwChannel, XL_BUS_TYPE_DAIO);
      return TRUE;
    }
  }

  // if we have some change, update it
  if (hwType) {
    xlSetApplConfig ("xlDAIOdemo", 0, hwType, hwIndex, hwChannel, XL_BUS_TYPE_DAIO);
    return TRUE;
  }
  return FALSE;
}

// start the channel with DAIO bus
void CXlDAIOdemoDlg::OnStart() {
  if (gChannelActivated) return;

  xlOpenDriver();

  if (OnFirstStart()) {
    xlPopupHwConfig (nullptr, 0);
    MessageBox ("Configuration was updated, please check the setting of \"DAIOdemo\" application channel.\n Press OK to proceed."); 
  }

  // now we should get correct application config
  unsigned int hwType, hwIndex, hwChannel; 
  if (xlGetApplConfig ("xlDAIOdemo",          // name of application's channel
                       0,                   // zero based application channel
                       &hwType,             // received type of hardware
                       &hwIndex,            // received hardware index
                       &hwChannel,          // received hardware channel
                       XL_BUS_TYPE_DAIO) 
                       != XL_SUCCESS) {
    AddMessage ("Error: xlGetApplConfig failed.");
    return;
  }

  // retrieve mask for selected channel
  gChannelMask = xlGetChannelMask (static_cast<int>(hwType), static_cast<int>(hwIndex), static_cast<int>(hwChannel));
  if (!gChannelMask){
    AddMessage ("Error: xlGetChannelMask failed.");
    return;
  }

  XLaccess initAccess = gChannelMask;

  // now we are going to open port for the application
  if (xlOpenPort ( &gPort,                // pointer to port handle
                   "xlDAIOdemo",            // application name
                   gChannelMask,          // selected channel mask
                   &initAccess,           // init access mask request / response
                   1024,                  // receive queue size
                   XL_INTERFACE_VERSION,  // desired xl interface version
                   XL_BUS_TYPE_DAIO)      // we are opening DAIO bus
                   != XL_SUCCESS) {
    AddMessage ("Error: xlOpenPort failed.");
    return;
  }

  // if we have not init access, we should fail here, because than we can only read DAIOdata
  if (gChannelMask != initAccess) {
    AddMessage ("Error: Application cannot get init access.");
    goto lerror;  // from now we should fail safely, because we could have opened port and driver
  }
  
  if (xlSetNotification (gPort, &gRxHandle, 1 ) != XL_SUCCESS) {
    AddMessage ("Error: xlSetNotification failed.");
    goto lerror;
  }

  // we can call configuration here to make sure that after activate channel we will have all settings done
  OnSetConfiguration();

  // following step is going to activate channel and stay prepared for configuration
  if (xlActivateChannel (gPort,                 // application port
                         initAccess,           // desired access mask
                         XL_BUS_TYPE_DAIO,      // we are activating DAIO bus
                         XL_ACTIVATE_RESET_CLOCK)  // timestamp should start from zero
                         != XL_SUCCESS) {
    AddMessage ("Error: xlActivateChannel failed.");
    goto lerror;
  }

  // if we have not init access, we should fail here, because than we can only read DAIOdata
  if (gChannelMask != initAccess) {
    AddMessage ("Error: Application cannot get init access for activation.");
    goto lerror;
  }

  // now we are ready with driver configuration, we have opened channel with DAIO configuration
  // and when bus configuration is updated, we can start measurement
  XLdriverConfig cfg;
  if (xlGetDriverConfig(&cfg) != XL_SUCCESS) {
    AddMessage ("Error: xlGetDriverConfig failed.");
    goto lerror;
  }

  // now we should display name of the channel and serial number of device we selected
  for (unsigned int pos = 0; pos < cfg.channelCount; pos++) {
    if (cfg.channel[pos].channelMask == gChannelMask){
      m_SelectedChannelName.Format("%s (s.n. %d)", cfg.channel[pos].name, cfg.channel[pos].serialNumber);
    }
  }
  
  // first reset all values to starting state
  m_DigitalStateList.ResetContent();
  std::for_each(begin(mDigitalOuts), end(mDigitalOuts), [](CButton& button) { button.SetCheck(0);});
  gLastDigital = 0;

  std::fill(std::begin(mAnalogs), std::end(mAnalogs), 0);
  std::fill(std::begin(gLastAnalog), std::end(gLastAnalog), 0);

  m_StartCtrl.EnableWindow (false);
  m_StopCtrl.EnableWindow (true);

  UpdateData(false);
  gChannelActivated = true;
  return;

lerror:
  xlClosePort(gPort);
  xlCloseDriver();
}

// stop the channel with DAIO bus
void CXlDAIOdemoDlg::OnStop() {
  if (!gChannelActivated) return;

  gRXThreadRun = false;

  WaitForSingleObject(gThreadHandle, 5000);

  // close driver configuration and free resources to other applications
  xlDeactivateChannel(gPort, gChannelMask);
  xlClosePort(gPort);
  xlCloseDriver();    

  gChannelActivated = false;
  m_SelectedChannelName = "No channel activated";
  m_StopCtrl.EnableWindow (false);
  m_StartCtrl.EnableWindow (true);
  UpdateData(false);
}

// send current configuration to the device
void CXlDAIOdemoDlg::OnSetConfiguration() {
  UpdateData(true);

  unsigned int measure_delay = 0;
  switch (m_MeasurementFrequency) {
    case 1: measure_delay = 1000;  //    1 Hz
      break;
    case 2: measure_delay =  100;  //   10 Hz
      break;
    case 3: measure_delay =   10;  //  100 Hz
      break;
    case 4: measure_delay =    1;  // 1000 Hz
  }

  if (xlDAIOSetMeasurementFrequency ( gPort, 
                                      gChannelMask,
                                      measure_delay)
                                      != XL_SUCCESS) {
    AddMessage ("Error: xlDAIOSetMeasurementFrequency failed.");
    return;
  }

  // now we will setup digital channels to output, values we can read anyway
  if (xlDAIOSetDigitalParameters (gPort, gChannelMask, 0x00, 0xff) != XL_SUCCESS) {
    AddMessage ("Error: xlDAIOSetDigitalParameters failed.");
    return;
  }

  // we will use two analog outputs (port 1 and 2) and two analog inputs
  // for all of them input range to 32V
  if (xlDAIOSetAnalogParameters (gPort, gChannelMask, 0x0c, 0x03, 0x0f) != XL_SUCCESS) {
    AddMessage ("Error: xlDAIOSetAnalogParameters failed.");
    return;
  }

  AddMessage ("Configuration was set successfully.");
}

// update analog outputs
void CXlDAIOdemoDlg::OnSetAnalog() {
  UpdateData(true);

  if (!gChannelActivated) {
    AddMessage ("Warning! OnSetAnalog: channel is not activated.");
    return;
  }

  if (xlDAIOSetAnalogOutput(gPort, gChannelMask, mAnalogs[0], mAnalogs[1], mAnalogs[2], mAnalogs[3]) != XL_SUCCESS) {
    AddMessage ("Error: OnSetAnalog: xlDAIOSetAnalogOutput failed.");
  }

}

// update digital outputs
void CXlDAIOdemoDlg::OnSetDigital1() {
  OnSetDigitalGeneric(0);
}

void CXlDAIOdemoDlg::OnSetDigital2() {
  OnSetDigitalGeneric(1);
}

void CXlDAIOdemoDlg::OnSetDigital3() {
  OnSetDigitalGeneric(2);
}

void CXlDAIOdemoDlg::OnSetDigital4() {
  OnSetDigitalGeneric(2);
}

void CXlDAIOdemoDlg::OnSetDigitalGeneric(unsigned int portNumber) {
  UpdateData(true);

  std::string message = "Warning! OnSetDigital" + std::to_string(portNumber + 1) + ": channel is not activated.";
  if (!gChannelActivated) {
    AddMessage(message.c_str());
    return;
  }
  unsigned int outPutMask = 1 << portNumber;
  unsigned int pattern = 0;
  if (mDigitalOuts[portNumber].GetCheck()) {
    pattern = outPutMask;
  }

  if (xlDAIOSetDigitalOutput(gPort, gChannelMask, outPutMask, pattern) != XL_SUCCESS) {
    AddMessage("Error: OnSetDigital1: xlDAIOSetDigitalOutput failed.");
    AddMessage(std::to_string(outPutMask).c_str());
    AddMessage(std::to_string(pattern).c_str());
  }
}

// manual measurement forced
void CXlDAIOdemoDlg::OnMeasureNow() {
  UpdateData(true);
  if (!gChannelActivated) {
    AddMessage ("Warning! OnMeasureNow: channel is not activated.");
    return;
  }

  if (xlDAIORequestMeasurement (gPort, gChannelMask) != XL_SUCCESS) {
    AddMessage ("Error: OnMeasureNow: xlDAIORequestMeasurement failed.");
  }
}

// pwm setting updated
void CXlDAIOdemoDlg::OnReleasedcaptureSlider1(NMHDR* pNMHDR, LRESULT* pResult) {
  UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = 0;
  UpdateData(true);
  if (!gChannelActivated) {
    AddMessage ("Warning! OnReleasedcaptureSlider1: channel is not activated.");
    return;
  }

  unsigned int frequency = 0;
  switch (m_PwmFrequency) {
    case 1:
      frequency = 100;
      break;    
    case 2:
      frequency = 5000;
      break;
    case 3: 
      frequency = 20000;
      break;
    case 4:
      frequency = 40000;
  }

  if (xlDAIOSetPWMOutput (gPort, gChannelMask, frequency, m_PwmValue) != XL_SUCCESS) {
    AddMessage ("Error: OnMeasureNow: xlDAIORequestMeasurement failed.");
  }
}