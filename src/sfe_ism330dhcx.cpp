#include "sfe_ism330dhcx.h"

//////////////////////////////////////////////////////////////////////////////
// init()
//
// init the system
//
// Return Value: false on error, true on success
//

bool QwDevISM330DHCX::init(void)
{

    if (_isInitialized)  // already init'd
        return true;

    //  do we have a bus yet? is the device connected?
    if (!_i2cBus || !_i2cAddress || !_i2cBus->ping(_i2cAddress))
        return false;

		// TO DO - check for WHOAMI
	
    _isInitialized = true;

    return true;
}



///////////////////////////////////////////////////////////////////////
// isConnected()
//
// Called to determine if a ISM330DHCX device, at the provided i2c address
// is connected.
//
//  Parameter   Description
//  ---------   -----------------------------
//  retval      true if device is connected, false if not connected

bool QwDevISM330DHCX::isConnected()
{
    return (_i2cBus && _i2cAddress ? _i2cBus->ping(_i2cAddress) : false);
}


////////////////////////////////////////////////////////////////////////////////////
// setCommunicationBus()
//
// Method to set the bus object that is used to communicate with the device
//
//  Parameter    Description
//  ---------    -----------------------------
//  theBus       The communication bus object
//  idBus        The id/address of the device on the bus

void QwDevISM330DHCX::setCommunicationBus(QwI2C &theBus, uint8_t idBus)
{
    _i2cBus = &theBus;
    _i2cAddress = idBus;
}

//////////////////////////////////////////////////////////////////////////////
//
//
int32_t QwDevISM330DHCX::writeRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length)
{
    return _i2cBus->writeRegisterRegion(_i2cAddress, offset, data, length);
}

int32_t QwDevISM330DHCX::readRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length)
{
    return _i2cBus->readRegisterRegion(_i2cAddress, offset, data, length);
}
