// qwiic_i2c.cpp
//
// This is a library written for SparkFun Qwiic ISM330DHCX boards
//
// SparkFun sells these bpards at its website: www.sparkfun.com
//
// Do you like this library? Help support SparkFun. Buy a board!
//
//SparkFun Qwiic 6DoF - ISM330DHCX        https://www.sparkfun.com/products/19764
//
// Written by Kirk Benell @ SparkFun Electronics 
// Modified by Elias Santistevan @ SparkFun Electronics, April 2022
//
// Repository:
//     https://github.com/sparkfun/SparkFun_6DoF_ISM330DHCX_Arduino_Library
//
//
// SparkFun code, firmware, and software is released under the MIT
// License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT
//
//    The MIT License (MIT)
//
//    Copyright (c) 2022 SparkFun Electronics
//    Permission is hereby granted, free of charge, to any person obtaining a
//    copy of this software and associated documentation files (the "Software"),
//    to deal in the Software without restriction, including without limitation
//    the rights to use, copy, modify, merge, publish, distribute, sublicense,
//    and/or sell copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following conditions: The
//    above copyright notice and this permission notice shall be included in all
//    copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
//    "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//    PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Class provide an abstract interface to the I2C device

#include "qwiic_i2c.h"
#include <Arduino.h>

// What is the max buffer size for this platform.

//#if defined(SERIAL_BUFFER_SIZE)
//#define kMaxTransferBuffer SERIAL_BUFFER_SIZE
//
//#elif defined(I2C_BUFFER_LENGTH)
//#define kMaxTransferBuffer I2C_BUFFER_LENGTH
//
//#elif defined(BUFFER_LENGTH)
//#define kMaxTransferBuffer BUFFER_LENGTH

#define kMaxTransferBuffer 32
#define SPI_READ 0x80


// What we use for transfer chunk size

const static uint16_t kChunkSize = kMaxTransferBuffer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor

QwI2C::QwI2C(void) : _i2cPort{nullptr}, _spiPort{nullptr}
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// init()
//
// Methods to init/setup this device. The caller can provide a Wire Port, or this class
// will use the default

bool QwI2C::init(TwoWire &wirePort, bool bInit)
{

    // if we don't have a wire port already
    if (!_i2cPort)
    {
        _i2cPort = &wirePort;

        if (bInit)
            _i2cPort->begin();
    }

    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//

//////////////////////////////////////////////////////////////////////////////////////////////////
//

bool QwI2C::init(void)
{
    // do we already have a wire port?
    if (!_i2cPort)
        return init(Wire, true); // no wire, send in Wire and init it

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ping()
//
// Is a device connected?
bool QwI2C::ping(uint8_t i2c_address)
{

    if (!_i2cPort)
        return false;

    _i2cPort->beginTransmission(i2c_address);
    return _i2cPort->endTransmission() == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// writeRegisterByte()
//
// Write a byte to a register

bool QwI2C::writeRegisterByte(uint8_t i2c_address, uint8_t offset, uint8_t dataToWrite)
{

    if (!_i2cPort)
        return false;

    _i2cPort->beginTransmission(i2c_address);
    _i2cPort->write(offset);
    _i2cPort->write(dataToWrite);
    return _i2cPort->endTransmission() == 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// writeRegisterRegion()
//
// Write a block of data to a device.

int QwI2C::writeRegisterRegion(uint8_t i2c_address, uint8_t offset, const uint8_t *data, uint16_t length)
{

    // Note:
    //      Because of how the TMF882X I2C works, you can't chunk over data - it must be
    //      sent in one transaction.
    //
    //      From an I2C standpoint, You can continue a write to the device over multiple
    //      transactions, just omitting the register (offset) after the first write transaction.
    //
    //      However, the data chunks for some elements of this (namely firmware uploads) have
    //      a checksum added to the data block being sent. This checksum is being validated
    //      by the device after each write transaction. If you chunk this across multi
    //      I2C transactions, it appears the checksum validation on the device fails, and
    //      the sensor/device won't enter "app mode" because upload failed.
    //
    //      To work around this, we reduce the chunk size for firmware upload in the file
    //      "inc/tmf882x_mode_bl.h" - the #define BL_NUM_DATA is adjusted based on the platform
    //      being used (what is it's I2C buffer size).

    // Just do a simple write transaction.

    _i2cPort->beginTransmission(i2c_address);
    _i2cPort->write(offset);
    _i2cPort->write(data, (int)length);

    return _i2cPort->endTransmission() ? -1 : 0; // -1 = error, 0 = success
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// readRegisterRegion()
//
// Reads a block of data from an i2c register on the devices.
//
// For large buffers, the data is chuncked over KMaxI2CBufferLength at a time
//
//
int QwI2C::readRegisterRegion(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t numBytes)
{
    uint8_t nChunk;
    uint16_t nReturned;

    if (!_i2cPort)
        return -1;

    // Note, this device handles *chunking* differently than others. Each chunk
    // is a transaction (stop conndition sent), but after the first read,
    // the next chunk is a standard I2C transaction, but you don't send the register
    // address

    int i;                   // counter in loop
    bool bFirstInter = true; // Flag for first iteration - used to send register

    while (numBytes > 0)
    {
        _i2cPort->beginTransmission(addr);

        if (bFirstInter)
        {
            _i2cPort->write(reg);
            bFirstInter = false;
        }

        if (_i2cPort->endTransmission() != 0)
            return -1; // error with the end transmission

        // We're chunking in data - keeping the max chunk to kMaxI2CBufferLength
        nChunk = numBytes > kChunkSize ? kChunkSize : numBytes;

        // For this device, we always send the stop condition - or it won't chunk data.
        nReturned = _i2cPort->requestFrom((int)addr, (int)nChunk, (int)true);

        // No data returned, no dice
        if (nReturned == 0)
            return -1; // error

        // Copy the retrieved data chunk to the current index in the data segment
        for (i = 0; i < nReturned; i++){
            *data++ = _i2cPort->read();
				}

        // Decrement the amount of data recieved from the overall data request amount
        numBytes = numBytes - nReturned;

    } // end while

    return 0; // Success
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// SPI init()
//
// Methods to init/setup this device. The caller can provide a Wire Port, or this class
// will use the default
bool SfeSPI::init(SPIClass &spiPort, SPISettings ismSPISettings, uint8_t cs, bool bInit)
{

    // if we don't have a wire port already
    if( !_spiPort )
    {
        _spiPort = &spiPort;

        if( bInit )
            _spiPort->begin();
    }


		if( !ismSPISettings )
			_sfeSPISettings = SPISettings(1000000, MSB_FIRST, SPI_MODE0);
		else
			_sfeSPISettings = ismSPISettings;

		if( !cs )
			return false; 
		
		_cs = cs;

    return true;
}


SfeSPI::SfeSPI(void) :  _spiPort{nullptr}
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// init()
//
// Methods to init/setup this device. The caller can provide a Wire Port, or this class
// will use the default

bool SfeSPI::init(SPIClass &spiPort, bool bInit)
{

    // if we don't have a wire port already
    if (!_spiPort)
    {
        _spiPort = &spiPort;

        if (bInit)
            _spiPort->begin();
    }

    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//

//////////////////////////////////////////////////////////////////////////////////////////////////
//

bool SfeSPI::init(void)
{
    // do we already have a wire port?
    if (!_spiPort)
        return init(SPI, true); // no wire, send in Wire and init it

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ping()
//
// Is a device connected?
bool SfeSPI::ping(uint8_t i2c_address)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// writeRegisterByte()
//
// Write a byte to a register

bool SfeSPI::writeRegisterByte(uint8_t i2c_address, uint8_t offset, uint8_t dataToWrite)
{

    if (!_spiPort)
        return false;

    _spiPort->beginTransaction(_sfeSPISettings);
		digitalWrite(_cs, LOW);
    _spiPort->transfer(offset);
    _spiPort->transfer(dataToWrite);
		digitalWrite(_cs, HIGH);
    _spiPort->endTransaction();

		return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// writeRegisterRegion()
//
// Write a block of data to a device.

int SfeSPI::writeRegisterRegion(uint8_t i2c_address, uint8_t offset, const uint8_t *data, uint16_t length)
{

		int i;

    _spiPort->beginTransaction(_sfeSPISettings);
		digitalWrite(_cs, LOW);
    _spiPort->transfer(offset);

		for(i = 0; i < length; i++)
		{
			_spiPort->transfer(*data++);
		}

		digitalWrite(_cs, HIGH);
    _spiPort->endTransaction();
		return 0; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// readRegisterRegion()
//
// Reads a block of data from the register on the device.
//
//
//
int SfeSPI::readRegisterRegion(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t numBytes)
{
    if (!_spiPort)
        return -1;

    int i;                   // counter in loop

    _spiPort->beginTransaction(_sfeSPISettings);
		digitalWrite(_cs, LOW);
		offset = offset | SPI_READ;
    _spiPort->transfer(offset);

		for(i = 0; i < numBytes; i++)
		{
			*data++ = _spiPort->transfer(0x00);
		}

		digitalWrite(_cs, HIGH);
    _spiPort->endTransaction();
		return 0; 

}
