#include "ddk768_reg.h"
#include "ddk768_power.h"
#include "ddk768_hwi2c.h"
#include "ddk768_help.h"

unsigned long hwI2CWriteData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
);

unsigned long hwI2CReadData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
);

static int ddk768_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
                           int num)
{
    struct smi_connector *connector = i2c_get_adapdata(adap);
    unsigned char i2cNumber = connector->i2cNumber;
    unsigned long ret;
    int i = 0;

    if(i2cNumber > 1)
    {
        return -EOPNOTSUPP;
    }

     for (i = 0; i < num; i++) {
		if (msgs[i].len == 0) {
			pr_err("unsupported transfer %d/%d, no data\n",
				i + 1, num);
			return -EOPNOTSUPP;
		}
	}


     for (i = 0; i < num; i++)
     {
		msgs->addr = msgs->addr << 1;

        if (msgs->flags & I2C_M_RD)
        {

		    ret = hwI2CReadData(
	                i2cNumber, // I2C0 or I2C1
	                msgs->addr,
	                msgs->len,
	                msgs->buf); 
		    if (ret < msgs->len)
	            {
					ret = 0;
	                pr_err("ddk768 i2c xfer rx failed %ld.\n", ret);
	                break;
	            }
	    }
        else
        {
            ret = hwI2CWriteData(
                i2cNumber, // I2C0 or I2C1
                msgs->addr,
                msgs->len,
                msgs->buf);
            if (ret < msgs->len)
            {
				ret = 0;
                pr_err("ddk768 i2c xfer tx failed %ld.\n", ret);
                break;
            }
        }

    	msgs++;
    }

     if (ret)
	 	ret = num;

        return ret;

}

static u32 ddk768_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}


const struct i2c_algorithm ddk768_i2c_algo = {
	.master_xfer	= ddk768_i2c_xfer,
	.functionality	= ddk768_i2c_func
};

/*
 *  This function initializes the hardware i2c
 *
 *  Return Value:
 *       0   - Success
 *      -1   - Fail to initialize i2c
 */
long ddk768_hwI2CInit(
    unsigned char i2cNumber //I2C0 or I2C1
)
{
    unsigned long value, offset;
    if(i2cNumber > 1) {
        return -1;
    }

    /* Enable GPIO pins as IIC clock & data */
    if (i2cNumber == 0)
    {
        value = FIELD_SET(peekRegisterDWord(GPIO_MUX), GPIO_MUX, I2C0, ENABLE);
        pokeRegisterDWord(GPIO_MUX, value);
        offset = 0;
    }
    else
    {
        value = FIELD_SET(peekRegisterDWord(GPIO_MUX), GPIO_MUX, I2C1, ENABLE);
        pokeRegisterDWord(GPIO_MUX, value);
        offset = I2C_OFFSET;
    }
              
    /* Enable the I2C Controller and set the bus speed mode */
    value = FIELD_SET(peekRegisterByte(I2C_CTRL+offset), I2C_CTRL, EN, ENABLE);
    value = FIELD_SET(value, I2C_CTRL, MODE, FAST);        
    pokeRegisterByte(I2C_CTRL+offset, value);

    return 0;
}

/*
 *  This function closes the hardware i2c.
 */
void ddk768_hwI2CClose(
    unsigned char i2cNumber //I2C0 or I2C1
)
{
    unsigned long value, offset;
    
    /* Set GPIO 30 & 31 back as GPIO pins */
    if (i2cNumber == 0)
    {
        value = FIELD_SET(peekRegisterDWord(GPIO_MUX), GPIO_MUX, I2C0, DISABLE);
        pokeRegisterDWord(GPIO_MUX, value);
        offset = 0;
    }
    else
    {
        value = FIELD_SET(peekRegisterDWord(GPIO_MUX), GPIO_MUX, I2C1, DISABLE);
        pokeRegisterDWord(GPIO_MUX, value);
        offset = I2C_OFFSET;
    }

    /* Disable I2C controller */
    value = FIELD_SET(peekRegisterByte(I2C_CTRL+offset), I2C_CTRL, EN, DISABLE);
    pokeRegisterByte(I2C_CTRL+offset, value);
}


/*
 *  This function waits until the transfer is completed within the timeout value.
 *
 *  Return Value:
 *       0   - Transfer is completed
 *      -1   - Tranfer is not successful (timeout)
 */
static long ddk768_hwI2CWaitTXDone(
    unsigned char i2cNumber //I2C0 or I2C1
)
{
    unsigned long timeout, offset;

    offset = (i2cNumber == 0)? 0 : I2C_OFFSET;

    /* Wait until the transfer is completed. */
    timeout = HWI2C_WAIT_TIMEOUT;
    while ((FIELD_VAL_GET(peekRegisterByte(I2C_STATUS+offset), I2C_STATUS, TX) != I2C_STATUS_TX_COMPLETED) &&
           (timeout != 0))
        timeout--;
    
    if (timeout == 0)
        return (-1);

    return 0;
}

/*
 *  This function writes data to the i2c slave device registers.
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address
 *      length          - Total number of bytes to be written to the device
 *      pBuffer         - The buffer that contains the data to be written to the
 *                     i2c device.   
 *
 *  Return Value:
 *      Total number of bytes those are actually written.
 */
unsigned long hwI2CWriteData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
)
{
    unsigned char count, i;
    unsigned long offset, totalBytes = 0;
    
    offset = (i2cNumber == 0)? 0 : I2C_OFFSET;

    /* Set the Device Address */
    pokeRegisterByte(I2C_SLAVE_ADDRESS+offset, deviceAddress & ~0x01);
    
    /* Write data.
     * Note:
     *      Only 16 byte can be accessed per i2c start instruction.
     */
    do
    {
        /* Reset I2C by writing 0 to I2C_RESET register to clear the previous status. */
        pokeRegisterByte(I2C_RESET+offset, 0);
        
        /* Set the number of bytes to be written */
        if (length < MAX_HWI2C_FIFO)
            count = length - 1;
        else
            count = MAX_HWI2C_FIFO - 1;
        pokeRegisterByte(I2C_BYTE_COUNT+offset, count);
        
        /* Move the data to the I2C data register */
        for (i = 0; i <= count; i++)
            pokeRegisterByte(I2C_DATA0 + i + offset, *pBuffer++);

        /* Start the I2C */
        pokeRegisterByte(I2C_CTRL+offset, FIELD_SET(peekRegisterByte(I2C_CTRL+offset), I2C_CTRL, CTRL, START));
        
        /* Wait until the transfer is completed. */
        if (ddk768_hwI2CWaitTXDone(i2cNumber) != 0)
            break;
    
        /* Substract length */
        length -= (count + 1);
        
        /* Total byte written */
        totalBytes += (count + 1);
        
    } while (length > 0);
            
    return totalBytes;
}

/*
 *  This function reads data from the slave device and stores them
 *  in the given buffer
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address
 *      length          - Total number of bytes to be read
 *      pBuffer         - Pointer to a buffer to be filled with the data read
 *                     from the slave device. It has to be the same size as the
 *                     length to make sure that it can keep all the data read.   
 *
 *  Return Value:
 *      Total number of actual bytes read from the slave device
 */
unsigned long hwI2CReadData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
)
{
    unsigned char count, i;
    unsigned long offset, totalBytes = 0; 
    
    offset = (i2cNumber == 0)? 0 : I2C_OFFSET;

    /* Set the Device Address */
    pokeRegisterByte(I2C_SLAVE_ADDRESS+offset, deviceAddress | 0x01);
    
    /* Read data and save them to the buffer.
     * Note:
     *      Only 16 byte can be accessed per i2c start instruction.
     */
    do
    {
        /* Reset I2C by writing 0 to I2C_RESET register to clear all the status. */
        pokeRegisterByte(I2C_RESET+offset, 0);
        
        /* Set the number of bytes to be read */
        if (length <= MAX_HWI2C_FIFO)
            count = length - 1;
        else
            count = MAX_HWI2C_FIFO - 1;
        pokeRegisterByte(I2C_BYTE_COUNT+offset, count);

        /* Start the I2C */
        pokeRegisterByte(I2C_CTRL+offset, FIELD_SET(peekRegisterByte(I2C_CTRL+offset), I2C_CTRL, CTRL, START));
        
        /* Wait until transaction done. */
        if (ddk768_hwI2CWaitTXDone(i2cNumber) != 0)
            break;

        /* Save the data to the given buffer */
        for (i = 0; i <= count; i++)
        {
            *pBuffer++ = peekRegisterByte(I2C_DATA0 + i + offset);
        }

        /* Substract length by 16 */
        length -= (count + 1);
    
        /* Number of bytes read. */
        totalBytes += (count + 1); 
        
    } while (length > 0);
    
    return totalBytes;
}

/*
 *  This function reads the slave device's register
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be read from
 *      registerIndex   - Slave device's register to be read
 *
 *  Return Value:
 *      Register value
 */
unsigned char ddk768_hwI2CReadReg(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned char registerIndex
)
{
    unsigned char value = (0xFF);

    if (hwI2CWriteData(i2cNumber, deviceAddress, 1, &registerIndex) == 1)
        hwI2CReadData(i2cNumber, deviceAddress, 1, &value);

    return value;
}

/*
 *  This function writes a value to the slave device's register
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be written
 *      registerIndex   - Slave device's register to be written
 *      data            - Data to be written to the register
 *
 *  Result:
 *          0   - Success
 *         -1   - Fail
 */
long ddk768_hwI2CWriteReg(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned char registerIndex, 
    unsigned char data
)
{
    unsigned char value[2];
    
    value[0] = registerIndex;
    value[1] = data;
    if (hwI2CWriteData(i2cNumber, deviceAddress, 2, value) == 2)
        return 0;

    return (-1);
}


long ddk768_AdaptHWI2CInit(struct smi_connector *connector)
{
    int ret;

    if (connector->base.connector_type == DRM_MODE_CONNECTOR_VGA)
    connector->i2cNumber = 1;
    else if (connector->base.connector_type == DRM_MODE_CONNECTOR_DVII)
    connector->i2cNumber = 0;
    else
    connector->i2cNumber = 2; // No exist.

    ddk768_hwI2CInit(connector->i2cNumber);

    connector->adapter.owner = THIS_MODULE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
    connector->adapter.class = I2C_CLASS_DDC;
#endif
    snprintf(connector->adapter.name, I2C_NAME_SIZE, "SMI HW I2C Bus");
    connector->adapter.dev.parent = connector->base.dev->dev;
    i2c_set_adapdata(&connector->adapter, connector);
	connector->adapter.algo = &ddk768_i2c_algo;
    ret = i2c_add_adapter(&connector->adapter);
	if (ret)
    {
        pr_err("HW i2c add adapter failed. %d\n", ret);
		return -1;
    }

    return 0;
}

long ddk768_AdaptHWI2CCleanBus(struct smi_connector *connector)
{
    ddk768_hwI2CClose(connector->i2cNumber);
    return 0;
}



