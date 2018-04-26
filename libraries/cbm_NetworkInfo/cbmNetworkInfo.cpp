#include <cbmNetworkInfo.h>

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
      strncpy ( ssid, "M5_IoT_MQTT", CBM_NI_STRLEN);
      strncpy ( password, "m5launch", CBM_NI_STRLEN);
      break;
      
    case ( CBMIoT ):
      strncpy ( ssid, "cbm_IoT_MQTT", CBM_NI_STRLEN);
      strncpy ( password, "cbmLaunch", CBM_NI_STRLEN);
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
  
    case ( CBMIoT ):
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
      ip   = { 172,  16,  10,  33 };
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

  // want to populate comments for MAC address and hex ID,
  // so using describe wherever we haven't populated them
  // and where we haven't yet verified them

  bool describe = false;

  #if defined ( ESP ) || defined ( ESP8266 )
    #ifdef ARDUINO_Heltec_WIFI_LoRa_32
      chipId = ESP.getEfuseMac();
    #else
      chipId = ESP.getChipId();
    #endif
  
    switch ( chipId ) {
    
      // ======================
      
      case 10648209UL:   // 0xa27a91  5C:CF:7F:A2:7A:91
        describe = true;
        strncpy ( chipName, "0", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 10;
        break;
      case 10806997UL:   // 0xa4e6d5  5C:CF:7F:A4:E6:D5
        describe = true;
        strncpy ( chipName, "1", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 11;
        break;
      case 15963942UL:   // 0xf39726  5C:CF:7F:F3:97:26
        describe = true;
        strncpy ( chipName, "3", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 13;
        break;
      case 122764UL:     // 0x01df8c  5C:CF:7F:01:DF:8C
        describe = true;
        strncpy ( chipName, "4", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 14;
        break;
      case 122085UL:     // 0x01dce5  5C:CF:7F:01:DC:E5
        describe = true;
        strncpy ( chipName, "5", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 15;
        break;
      case 16061816UL:   // 0xf51578  5C:CF:7F:F5:15:78
        describe = true;
        strncpy ( chipName, "6", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 16;
        break;
      case 121802UL:     // 0x01dbca  5C:CF:7F:01:DB:CA
        describe = true;
        strncpy ( chipName, "7", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 17;
        break;
      case 121932UL:     // 0x01dc4c  5C:CF:7F:01:DC:4C
        describe = true;
        strncpy ( chipName, "8", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 18;
        break;
      case 8564065UL:    // 0x82ad61  5C:CF:7F:82:AD:61
        strncpy ( chipName, "9", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 19;
        break;
      case 8562041UL:    // 0x82a579  5C:CF:7F:82:A5:79
        describe = true;
        strncpy ( chipName, "10", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 20;
        break;
      case 8566400UL:    // 0x82b680  5C:CF:7F:82:B6:80
        describe = true;
        strncpy ( chipName, "11", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 21;
        break;
      case 1373344UL:    // 0x14f4a0  5C:CF:7F:14:F4:A0
        describe = true;
        strncpy ( chipName, "12", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 22;
        break;
      case 1167901UL:    // 0x11D21D  5C:CF:7F:11:D2:1D
        strncpy ( chipName, "13", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 23;
        break;
      case 1122545UL:    // 0x1120f1  5C:CF:7F:11:20:F1
        describe = true;
        strncpy ( chipName, "14", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 24;
        break;
      case 1126382UL:    // 0x112fee  5C:CF:7F:11:2F:EE
        strncpy ( chipName, "15", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 25;
        break;
      case 1167824UL:    // 0x11D1D0  5C:CF:7F:11:D1:D0
        strncpy ( chipName, "16", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 26;
        break;
      case 1168735UL:    // 0x11D55F  5C:CF:7F:11:D5:5F
        strncpy ( chipName, "17", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 27;
        break;
      case 1171953UL:    // 0x11E1F1  5C:CF:7F:11:E1:F1
        strncpy ( chipName, "18", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 28;
        break;
      case 1121341UL:    // 0x111C3D  5C:CF:7F:11:1C:3D
        strncpy ( chipName, "19", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 29;
        break;

      // ======================
      // ====== Arnica ========
      // ======================
      
      case 15860301UL:   // 0xf2024d
        describe = true;
        strncpy ( chipName, "A00", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 40;
        break;
      case 15860098UL:   // 0xf20182
        describe = true;
        strncpy ( chipName, "A01", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 41;
        break;
      case 15860679UL:   // 0xf203c7
        describe = true;
        strncpy ( chipName, "A02", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 42;
        break;
      case 15859049UL:   // 0xf1fd69  18:FE:34:F1:FD:69
        strncpy ( chipName, "A03", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 43;
        break;
        
      // ======================
      // ======= Huzzah =======
      // ======================
      
      case 16045133UL:   // 0xf4d44d
        describe = true;
        strncpy ( chipName, "H0", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 70;
        break;
      case 89095UL:      // 0x015c07
        describe = true;
        strncpy ( chipName, "H1", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 71;
        break;
      case 16051230UL:   // 0xf4ec1e
        describe = true;
        strncpy ( chipName, "H2", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 72;
        break;
      case 133374UL:     // 0x0208fe  5C:CF:7F:02:08:FE
        strncpy ( chipName, "H4", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 74;
        break;
      case 16051132UL:   // 0xf4ebbc
        describe = true;
        strncpy ( chipName, "H5", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 75;
        break;
      case 16616310UL:   // 0xfd8b76
        describe = true;
        strncpy ( chipName, "H6", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 76;
        break;
        
      // ======================
      // ========= s ==========
      // ======================
      
      case 3937980UL:    // 0x3c16bc
        describe = true;
        strncpy ( chipName, "s0", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 100;
        break;
      case 16360913UL:   // 0xf9a5d1
        describe = true;
        strncpy ( chipName, "s1", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 101;
        break;
      case 3919261UL:    // 0x3bcd9d
        describe = true;
        strncpy ( chipName, "s3", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 103;
        break;
        
      // ======================
      // ========= l ==========
      // ======================
      
      case 0x50e643a4ae30:
        describe = true;
        strncpy ( chipName, "l0", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 150;
        break;
      case 0xd8e343a4ae30:
        describe = true;
        strncpy ( chipName, "l1", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 151;
        break;
        
      // ======================
      // ======================
      // ======================
      
      default:
        describe = true;
        strncpy ( chipName, "unknown", CBM_NI_CHIPNAME_STRLEN);
        ip [ 3 ] = 254;
        break;
    }
    if ( describe ) describeESP ( chipName );
    yield ();
  #endif
}

// shall we do something like "getBroadcastAddress" ?

void cbmNetworkInfo::describeESP ( char* chipName ) {
  Serial.print ( "\n\n  --\n" ); 
  #if defined ( ESP ) || defined ( ESP8266 )
    #ifdef ARDUINO_Heltec_WIFI_LoRa_32
      uint64_t cid = ESP.getEfuseMac ();
      Serial.print ( "  Heltec_WIFI_LoRa_32 Chip Name: " ); Serial.print ( chipName );
      Serial.printf ( "; Chip ID: 0x%04x%08x",
         (uint16_t) ( cid >> 32 ),
         (uint32_t) ( cid ) );
    #else
      Serial.print ( "  ESP Chip Name: " ); Serial.print ( chipName );
      Serial.print ( "; Chip ID: " ); Serial.print ( ESP.getChipId () );
      Serial.print ( " ( 0x" ); Serial.print ( ESP.getChipId (), HEX );
      Serial.print ( " )" );
    #endif
    Serial.print ( "; MAC address: " ); Serial.println ( WiFi.macAddress() );

    Serial.print ( "  Flash chip size: " ); Serial.print ( ESP.getFlashChipSize() );
    Serial.print ( " / " ); Serial.print ( ESP.getFlashChipMode() );
    Serial.print ( " (enum) at " ); Serial.print ( ESP.getFlashChipSpeed() / 1000000UL ); Serial.println ( " MHz" );
    Serial.print ( "  --" ); Serial.println ();
  #else
    Serial.print ( "\n\n\ncbmNetworkInfo ERROR: \n       ESP and ESP8266 are both not defined!\n\n\n" );
  #endif
}

void cbmNetworkInfo::describeNetwork () {
  /*
    use after
      WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
      WiFi.begin ( Network.ssid, Network.password );
  */
  Serial.print ( "\n\nNetwork Characteristics:\n" ); 
  Serial.print ( "  SSID/pw: " ); Serial.print ( ssid ); 
  Serial.print ( " / " ); Serial.print ( password );
  Serial.print ( "\n" );
  Serial.print ( "  ip: " );   printIP ( ip );
  Serial.print ( "  gw: " );   printIP ( gw ); 
  Serial.print ( "  mask: " ); printIP ( mask );
  Serial.print ( "  dns: " );  printIP ( dns );
  Serial.print ( "\n\n" );
}

void cbmNetworkInfo::printIP ( IPAddress ip ) {
  Serial.print ( ip [ 0 ] );
  for ( int i = 1; i < 4; i++ ) {
    Serial.print ( "." );
    Serial.print ( ip [ i ] );
  }
}