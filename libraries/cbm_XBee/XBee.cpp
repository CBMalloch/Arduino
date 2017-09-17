/*
	XBee.cpp - library for working with XBee API packets
	Created by Charles B. Malloch, PhD, March 31, 2011
	Released into the public domain
*/

#include "XBee.h"

int _nXBeePackets = 0;

XBeePacket::XBeePacket() {
	reset();
  _nXBeePackets++;
}

XBeePacket::XBeePacket(byte* packet) {
	reset();
  _nXBeePackets++;
  _interpret(packet);
}

XBeePacket::~XBeePacket() {
  _nXBeePackets--;
}

void XBeePacket::_interpret(byte* packet) {
  int len;
  _packet_status = 0;
  if (packet[0] == API_PACKET_START_DELIMITER) {
    // it's an API packet as desired
    _lengthMSB = packet[1];
    _lengthLSB = packet[2];
    len = length();
    if (len >  maxLenXBeePacket) {
      len = maxLenXBeePacket;
    }
    
    _payload = packet + 3;
    // _payload = (byte *) malloc ((len + 1) * sizeof(byte));
    // // note we can't use strncpy since the packet will probably contain nulls!
    // for (int i = 0; i < len + 1; i++) {
      // _payload[i] = packet[i+3];
    // }
    _packet_status = 1;
    _validate_packet();
  }
}

void XBeePacket::_validate_packet() {
  byte check = 0;
  // packet length field excludes header (1 byte), length (2), and checkum (1)
  // but _payload points past the header and length
  for ( int i = 0; i < length() + 1; i++ ) { 
    char c = _payload [ i ];
    check += int ( c );
  }
  if ( check == 0xff ) {
    _packet_status = 2;
  } else {
    _packet_status = 0x4000 | check;
  }
}

int XBeePacket::nXBeePackets() {
  return (_nXBeePackets);
}

int XBeePacket::nXBeePackets(int nXBeePackets) {
  _nXBeePackets = nXBeePackets;
  return (_nXBeePackets);
}

unsigned int XBeePacket::length() {
  return ( ( _lengthMSB << 8) + _lengthLSB );
}

int XBeePacket::packet_status() {
  return int(_packet_status);
}

const char *XBeePacket::version() {
	return "0.001.000";
}

void XBeePacket::reset() {
	_packet_status = -1;
  _lengthMSB = 0;
  _lengthLSB = 0;
}

// ************************************************************************
// ************************************************************************
// ************************************************************************

SERIES1_IO_Packet::SERIES1_IO_Packet(byte* payload) {
  _payload = payload;
  _total_samples = _payload[5];
  _channel_indicator = (_payload[6] << 8) + _payload[7];
  _nDigitalChannels = 0;
  for (int i = 0; i < 9; i++) {
    if (_channel_indicator & (0x01 << i)) {
      _nDigitalChannels++;
    }
  }
  _nAnalogChannels = 0;
  for (int i = 0; i < 6; i++) {
    if (_channel_indicator & (0x01 << (i + 9))) {
      _analog_channel_offset[i] = _nAnalogChannels * 2;
      _nAnalogChannels++;
    } else {
      _analog_channel_offset[i] = 0xff;
    }
  }
  _sequence_length = ((_nDigitalChannels > 0) ? 2 : 0) + 2 * _nAnalogChannels;
}

SERIES1_IO_Packet::~SERIES1_IO_Packet() {
}

const char *SERIES1_IO_Packet::version() {
	return "0.000.001";
}

bool SERIES1_IO_Packet::is_SERIES1_IO_Packet() {
  return (_payload[0] == SERIES1_IO_PACKET_START_DELIMITER);
}
    
int SERIES1_IO_Packet::address_16() {
  return ((_payload[1] << 8) + _payload[2]);
}

int SERIES1_IO_Packet::rssi() {
  return _payload[3];
}

bool SERIES1_IO_Packet::is_address_broadcast() {
  return (_payload[4] & 0x02);
}

bool SERIES1_IO_Packet::is_pan_broadcast() {
  return (_payload[4] & 0x04);
}

int SERIES1_IO_Packet::total_samples() {
  return (_total_samples);
}

unsigned int SERIES1_IO_Packet::channel_indicator() {
  return (_channel_indicator);
}

unsigned int SERIES1_IO_Packet::nDigitalChannels() {
  return (_nDigitalChannels);
}

unsigned int SERIES1_IO_Packet::nAnalogChannels() {
  return (_nAnalogChannels);
}

bool SERIES1_IO_Packet::data_request_was_invalid() {
  return (_data_request_was_invalid);
}

int SERIES1_IO_Packet::_SOS_offset(int n) {
  // offset to start of sequence
  // sequences start at byte 8 of the payload
  // the length of each is 2 * <there is digital data> + 2 * _nAnalogChannels
  // returns 0 if n is out of bounds
  _data_request_was_invalid = 0;
  if (n < 0 || n >= _total_samples) {
    _data_request_was_invalid = 1;
    return (0x00);
  }
  return (8 + n * _sequence_length);
}

bool SERIES1_IO_Packet::digitalReading(int channel, int sequence) {
  _data_request_was_invalid = 0;
  if (    sequence < 0 
      ||  sequence >= _total_samples 
      ||  channel < 0
      ||  channel >= 9) {
    _data_request_was_invalid = 1;
    return (0x00);
  }
  int within_seq_offset = (channel > 7) ? 0 : 1;
  channel &= 0xff;
  return _payload[_SOS_offset(sequence) + within_seq_offset] & (0x01 << channel);
}

int SERIES1_IO_Packet::analogReading(int channel, int sequence) {
  _data_request_was_invalid = 0;
  if (    sequence < 0 
      ||  sequence >= _total_samples 
      ||  channel < 0
      ||  channel >= 6
      ||  _analog_channel_offset[channel] == 0xff) {
    _data_request_was_invalid = 1;
    return (0x00);
  }
  int byte_offset = _SOS_offset(sequence) 
                  + _analog_channel_offset[channel]
                  + ((_nDigitalChannels > 0) ? 2 : 0);
  // return (byte_offset);  // for debugging
  return (_payload[byte_offset] << 8) | _payload[byte_offset+1];
}