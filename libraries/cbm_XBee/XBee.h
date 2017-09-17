/*
	XBee.h - library for working with XBee API packets
	Created by Charles B. Malloch, PhD, March 31, 2011
	Released into the public domain
   
*/

#ifndef XBee_h
#define XBee_h

typedef unsigned char byte;

class XBeePacket {

public:

  #define API_PACKET_START_DELIMITER 0x7e
  XBeePacket ();
  XBeePacket ( byte* packet );
  ~XBeePacket ();
  const char *version ();
  void reset ();
  
  int nXBeePackets ();
  int nXBeePackets ( int nXBeePackets );

  unsigned int length ();
  int packet_status ();
  
private:  // comment out this line for debugging...
  
  #define maxLenXBeePacket 256               //  TO BECOME PRIVATE AFTER DEBUGGING
  byte* _payload;
  
  void _interpret ( byte* packet );
  void _validate_packet ();
  
  byte
        _PACKET_START,
        _lengthMSB,
        _lengthLSB
      ;
  // _packet_status values:
  // -1: reset
  //  0: packet start delimiter mismatch
  //  1: packet checksum mismatch
  //  2: packet valid
  short _packet_status;
};

// ************************************************************************
// ************************************************************************
// ************************************************************************

class SERIES1_IO_Packet {

public:

  SERIES1_IO_Packet ();
  SERIES1_IO_Packet ( byte* payload );
  ~SERIES1_IO_Packet ();
  const char *version ();
  
  bool is_SERIES1_IO_Packet ();
  int address_16 ();
/*
    # Received Signal Strength Indicator - Hexadecimal equivalent of (-dBm) value.
    # For example: If RX signal strength = -40 dBm, 0x28 (40 decimal) is returned
    # Parameter Range : 0x17-0x5C (-23dBm to -92dBm) (XBee), 0x24-0x64 (-36dBm to -100dBm) (XBee-PRO)
    # dBm (sometimes dBmW) is an abbreviation for the power ratio in decibels (dB) 
    # of the measured power referenced to one milliwatt (mW).
*/
  int rssi ();
  bool is_address_broadcast ();
  bool is_pan_broadcast ();

  int total_samples ();
  bool data_request_was_invalid ();
  unsigned int channel_indicator ();
  unsigned int nDigitalChannels ();
  unsigned int nAnalogChannels ();
  bool digitalReading ( int channel, int sequence );
  int analogReading ( int channel, int sequence );
  
private:  // comment out this line for debugging...

  #define SERIES1_IO_PACKET_START_DELIMITER 0x83   // RX (Receive) Packet: 16-bit address I/O
  
  // I define payload as what's left after stripping the API packet of its 3-byte header
  // It retains the checksum byte at the end
  byte* _payload;
  bool _data_request_was_invalid;
  unsigned int _channel_indicator;
  int _total_samples;
  int _nDigitalChannels, _nAnalogChannels;
  int _analog_channel_offset [ 6 ];
  int _sequence_length;  // the length, in bytes, of one IO measurement sequence
  int _SOS_offset ( int n );  // offset to start of IO measurement sequence n
};

/*
class XBee {
for future expansion; this list is the configuration parameters for the XBee
private:
	byte  ID, // PAN_ID
				_DH, // destination_address_high_byte
				_DL,  // destination_address_low_byte
				_MY,  // source_address
				_AI,  // association_indication
				_NI,  // node_identifier
				_PL,  // power_level
				_SM,  // sleep_mode
				_ST,  // time_before_sleep
				_SP,  // cyclic_sleep_period
				_BD,  // interface_data_rate
				_NB,  // parity
				_RO,  // packetization timeout
				_AP,  // API_enable
				      // IO_settings
				_IU,  // IO_output_enable
				_IT,  // samples_before_TX
				_IC,  // DIO_change_detect
				_IR,  // sample_rate
				_VR,  // firmware_version
				_HV,  // hardware_version
				_DB,  // received_signal_strength
      ;
};
*/

#endif
