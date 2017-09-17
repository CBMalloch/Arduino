#include <cbmNetworkInfo.h>
#include <ESP8266WiFi.h>

cbmNetworkInfo::cbmNetworkInfo() {
}

void cbmNetworkInfo::init ( int wifi_locale ) {
  setCredentials ( wifi_locale );
  setNetworkBaseIP ( wifi_locale );
  fixIP ( );
}
      
  // WiFi.begin is said to take less time if DHCP is not used.
  // We set a fixed IP address by calling WiFi.config before calling WiFi.begin
  
void cbmNetworkInfo::setCredentials ( int wifi_locale ) {

  switch ( wifi_locale ) {
    
    case ( M5 ):
      strncpy ( ssid, "M5", CBM_NI_STRLEN);
      strncpy ( password, "", CBM_NI_STRLEN);
      break;
      
    case ( CBMM5 ):
      strncpy ( ssid, "M5", CBM_NI_STRLEN);
      strncpy ( password, "m5launch", CBM_NI_STRLEN);
      break;
    
    case ( CBMDATACOL ):
      strncpy ( ssid, "cbmDataCollection", CBM_NI_STRLEN);
      strncpy ( password, "cbmTennessees", CBM_NI_STRLEN);
      break;
  
    case ( CBMDDWRT ):
      strncpy ( ssid, "dd-wrt", CBM_NI_STRLEN);
      strncpy ( password, "cbmAlaskas", CBM_NI_STRLEN);
      break;
    
    case ( CBMDDWRT3 ):
      strncpy ( ssid, "dd-wrt-03", CBM_NI_STRLEN);
      strncpy ( password, "cbmAlaskas", CBM_NI_STRLEN);
      break;
    
    case ( CBMDDWRT3GUEST ):
      strncpy ( ssid, "dd-wrt-03-guest", CBM_NI_STRLEN);
      strncpy ( password, "cbmGuest", CBM_NI_STRLEN);
      break;
    
    case ( CBMBELKIN ):
      strncpy ( ssid, "cbmBelkin54g-2", CBM_NI_STRLEN);
      strncpy ( password, "belkinAlaskas", CBM_NI_STRLEN);
      break;
  
    case ( CBM_RASPI_MOSQUITTO ):
  
      strncpy ( ssid, "dd-wrt", CBM_NI_STRLEN);
      strncpy ( password, "cbmAlaskas", CBM_NI_STRLEN);
      break;
  
    default: 
    
      strncpy ( ssid, "default", CBM_NI_STRLEN);
      strncpy ( password, "default", CBM_NI_STRLEN);
      break;

  }  // switch
  
}

void cbmNetworkInfo::setNetworkBaseIP ( int wifi_locale ) {
      
  // WiFi.begin is said to take less time if DHCP is not used.
  // We set a fixed IP address by calling WiFi.config before calling WiFi.begin

  switch ( wifi_locale ) {
    
    case ( M5 ):
      ip   = { 192, 168,   5, 33 };
      gw   = { 192, 168,   5,  1 };
      mask = { 255, 255, 255,  0 };
      dns  = { 192, 168,   5,  1 };
      break;
  
    case ( CBMM5 ):
      ip   = { 192, 168,   5, 33 };
      gw   = { 192, 168,   5,  1 };
      mask = { 255, 255, 255,  0 };
      dns  = { 192, 168,   5,  1 };
      break;
    
    case ( CBMDATACOL ):
      ip   = { 172,  16,   5,  33 };
      gw   = { 172,  16,  68,   3 };
      mask = { 255, 255,   0,   0 };
      dns  = { 172,  16,  68,   3 };
      break;
  
    case ( CBMDDWRT3 ):
      ip   = { 172,  16,   5,  33 };
      gw   = { 172,  16,  68,   3 };
      mask = { 255, 255,   0,   0 };
      dns  = { 172,  16,  68,   3 };
      break;
    
    case ( CBMDDWRT3GUEST ):
      ip   = { 192, 168,  10,  33 };
      gw   = { 192, 168,  10,   1 };
      mask = { 255, 255, 255,   0 };
      dns  = { 192, 168,  10,   1 };
      break;
    
    case ( CBM_RASPI_MOSQUITTO ):
      ip   = { 172,  16,   5,  33 };
      gw   = { 172,  16,  68,   3 };
      mask = { 255, 255,   0,   0 };
      dns  = { 172,  16,  68,   3 };
      break;
  
    case ( CBMDDWRT ):
      ip   = { 172,  16,   5,  33 };
      gw   = { 172,  16,  69,   2 };
      mask = { 255, 255,   0,   0 };
      dns  = { 172,  16,  69,   2 };
      break;
    
    case ( CBMBELKIN ):
      ip   = { 172,  16,   5,  33 };
      gw   = { 172,  16,  69,   2 };
      mask = { 255, 255,   0,   0 };
      dns  = { 172,  16,  69,   2 };
      break;
  
    default:
      ip   = {   0,   0,   0,   0 };
      gw   = {   0,   0,   0,   0 };
      mask = {   0,   0,   0,   0 };
      dns  = {   0,   0,   0,   0 };
      break;

  }  // switch
  
}

void cbmNetworkInfo::fixIP ( ) {
  chipId = ESP.getChipId();
  switch ( chipId ) {
    // ======================
    case 10648209UL:
      strncpy ( chipName, "0", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 10;
      break;
    case 10806997UL:
      strncpy ( chipName, "1", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 11;
      break;
    case 15963942UL:
      strncpy ( chipName, "3", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 13;
      break;
    case 122764UL:
      strncpy ( chipName, "4", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 14;
      break;
    case 122085UL:
      strncpy ( chipName, "5", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 15;
      break;
    case 16061816UL:
      strncpy ( chipName, "6", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 16;
      break;
    case 121802UL:
      strncpy ( chipName, "7", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 17;
      break;
    case 121932UL:
      strncpy ( chipName, "8", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 18;
      break;
    case 8564065UL:
      strncpy ( chipName, "9", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 19;
      break;
    case 8562041UL:
      strncpy ( chipName, "10", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 20;
      break;
    case 8566400UL:
      strncpy ( chipName, "11", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 21;
      break;
    case 1373344UL:
      strncpy ( chipName, "12", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 22;
      break;
    // ======================
    case 15860301UL:
      strncpy ( chipName, "A00", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 40;
      break;
    case 15860098UL:
      strncpy ( chipName, "A01", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 41;
      break;
    case 15860679UL:
      strncpy ( chipName, "A02", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 42;
      break;
    case 15859049UL:
      strncpy ( chipName, "A03", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 43;
      break;
    // ======================
    case 16045133UL:
      strncpy ( chipName, "H0", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 70;
      break;
    case 89095UL:
      strncpy ( chipName, "H1", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 71;
      break;
    case 16051230UL:
      strncpy ( chipName, "H2", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 72;
      break;
    case 133374UL:
      strncpy ( chipName, "H4", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 74;
      break;
    case 16051132UL:
      strncpy ( chipName, "H5", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 75;
      break;
    case 16616310UL:
      strncpy ( chipName, "H6", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 76;
      break;
    // ======================
    case 3937980UL:
      strncpy ( chipName, "s0", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 100;
      break;
    case 16360913UL:
      strncpy ( chipName, "s1", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 101;
      break;
    case 3919261UL:
      strncpy ( chipName, "s3", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 103;
      break;
    // ======================
    default:
      strncpy ( chipName, "unknown", CBM_NI_CHIPNAME_STRLEN);
      ip [ 3 ] = 254;
      break;
  }
  yield ();
}

// shall we do something like "getBroadcastAddress" ?

void cbmNetworkInfo::describeESP ( char* chipName, HardwareSerial *strSerial ) {
  strSerial->print ( "\n\n--" ); 
  strSerial->print ( " Chip: " ); strSerial->print ( chipName );
  strSerial->print ( " Chip ID: " ); strSerial->print ( ESP.getChipId () );
  strSerial->print ( " ( 0x" ); strSerial->print ( ESP.getChipId (), HEX );
  strSerial->print ( " ) --" ); strSerial->println ();

  strSerial->print ( "Flash chip size: " ); strSerial->print ( ESP.getFlashChipSize() );
  strSerial->print ( " / " ); strSerial->print ( ESP.getFlashChipMode() );
  strSerial->print ( " (enum) at " ); strSerial->print ( ESP.getFlashChipSpeed() / 1000000UL ); strSerial->println ( " MHz" );
}

void cbmNetworkInfo::describeNetwork ( HardwareSerial *strSerial ) {
  /*
    use after
      WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
      WiFi.begin ( Network.ssid, Network.password );
  */
  strSerial->print ( "\n\nNetwork Characteristics:\n" ); 
  strSerial->print ( "  SSID/pw: " ); strSerial->print ( ssid ); 
  strSerial->print ( " / " ); strSerial->print ( password );
  strSerial->print ( "\n" );
  strSerial->print ( "  ip: " );   printIP ( ip );
  strSerial->print ( "  gw: " );   printIP ( gw ); 
  strSerial->print ( "  mask: " ); printIP ( mask );
  strSerial->print ( "  dns: " );  printIP ( dns );
  strSerial->print ( "\n\n" );
}

void cbmNetworkInfo::printIP ( IPAddress ip, HardwareSerial *strSerial ) {
  strSerial->print ( ip [ 0 ] );
  for ( int i = 1; i < 4; i++ ) {
    strSerial->print ( "." );
    strSerial->print ( ip [ i ] );
  }
}