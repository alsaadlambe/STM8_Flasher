

/**********************Device Specific File************************/
/**********************Device Specific File************************/
/**********************Device Specific File************************/
/**********************Device Specific File************************/

#include "stm8s003_swim.h"
#include "stm8_swim.h"
#include "millis.h"
#include "at24cxx.h"

uint8_t RAM_Buffer[STM8S003_BLOCK_SIZE];
uint8_t Compare_Buffer[STM8S003_BLOCK_SIZE];

uint8_t STM8S003_Default_OPT[10] = {0x00, 0xFF,
                                    0x00, 0xFF,
                                    0x00, 0xFF,
                                    0x00, 0xFF,
                                    0x00, 0xFF};

uint8_t SWIM_Unlock_OptionByte(void)
{
  uint8_t temp[2];
  temp[0] = 0x80; // OPT = 1 and NOPT = 0
  temp[1] = 0x7F;

  if (SWIM_WOTF(SWIM_FLASH_CR2, temp, 2))
  {
    return SWIM_Unlock_EEPROM(); //opt unlock sequence
  }

  return 0;
}

uint8_t SWIM_Lock_OptionByte()
{
  uint8_t temp[2];
  temp[0] = 0x7F; // OPT = 0 and NOPT = 1
  temp[1] = 0x80; // enable opt

  if (SWIM_WOTF(SWIM_FLASH_CR2, temp, 2))
  {
    return SWIM_Lock_EEPROM();
  }
  return 0;
}

uint8_t SWIM_Unlock_EEPROM()
{
  uint8_t temp[1];
  temp[0] = SWIM_FLASH_DUKR_KEY1;
  if (SWIM_WOTF(SWIM_FLASH_DUKR, temp, 1))
  {
    temp[0] = SWIM_FLASH_DUKR_KEY2;
    return SWIM_WOTF(SWIM_FLASH_DUKR, temp, 1);
  }
  return 0;
}

uint8_t SWIM_Lock_EEPROM()
{
  uint8_t temp[1];
  if (SWIM_ROTF(SWIM_FLASH_IAPSR, temp, 1))
  {
    temp[0] &= (uint8_t)0xF7; //
    return SWIM_WOTF(SWIM_FLASH_IAPSR, temp, 1);
  }
  return 0;
}

uint8_t SWIM_Unlock_Flash()
{
  uint8_t temp[1];
  temp[0] = SWIM_FLASH_PUKR_KEY1;
  if (SWIM_WOTF(SWIM_FLASH_PUKR, temp, 1))
  {
    temp[0] = SWIM_FLASH_PUKR_KEY2;
    return SWIM_WOTF(SWIM_FLASH_PUKR, temp, 1);
  }
  return 0;
}

uint8_t SWIM_Lock_Flash()
{
  uint8_t temp[1];
  if (SWIM_ROTF(SWIM_FLASH_IAPSR, temp, 1))
  {
    temp[0] &= 0xFD; //
    return SWIM_WOTF(SWIM_FLASH_IAPSR, temp, 1);
  }
  return 0;
}

uint8_t SWIM_Enable_Block_Programming()
{
  uint8_t temp[2];
  temp[0] = 0x01; //Flash_CR2  standard block programming
  temp[1] = 0xFE; //Flash_NCR2
  return SWIM_WOTF(SWIM_FLASH_CR2, temp, 2);
}

uint8_t SWIM_Wait_For_EOP()
{
  uint8_t flagstatus[1] = {0};
  uint8_t timeout = 0xFF;

  while ((flagstatus[0] == 0x00) && --timeout)
  {
    delay_us(500);
    SWIM_ROTF(SWIM_FLASH_IAPSR, flagstatus, 1);
    if (flagstatus[0] & FLASH_IAPSR_WR_PG_DIS)
    {
      return 0;
    }
    flagstatus[0] = (uint8_t)(flagstatus[0] & FLASH_IAPSR_EOP);
  }

  if (timeout)
  {
    return 1;
  }

  return 0;
}

uint8_t SWIM_Enable_Read_Out_Protection()
{
  uint8_t temp[1];
  temp[0] = 0xAA;
  if (SWIM_Unlock_OptionByte())
  {
    if (SWIM_WOTF(SWIM_OPT0, temp, 1))
    {
      return SWIM_Wait_For_EOP();
    }
  }
  return 0;
}

uint8_t SWIM_Disable_Read_Out_Protection()
{
  uint8_t temp[1];
  temp[0] = 0x00;
  if (SWIM_Unlock_OptionByte())
  {
    if (SWIM_WOTF(SWIM_OPT0, temp, 1))
    {
      return SWIM_Wait_For_EOP();
    }
  }
  return 0;
}

/***** Clone stm8s003  ****/
uint8_t Copy_STM8S003_To_AT24CXX(void)
{
  uint8_t status;
  uint16_t stm8s003_mem_address;
  uint16_t at24xx_mem_address;

  status = SWIM_Enter();

  /****************************read flash data from stm8 start********************************/

  stm8s003_mem_address = STM8_FLASH_START_ADDRESS;
  at24xx_mem_address = FLASH_STORE_ADDRESS;

  for (uint8_t i = 0; i < STM8S003_FLASH_PAGES; i++)
  {
    if (status)
    {
      status = SWIM_ROTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
    }
    if (status)
    {
      //status=AT24CXX_Write_Page(at24xx_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
      //delay_ms(3);// read from stm8 takes 2.9ms // wait for EOP
    }
    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_GREEN_TOGGLE(); // signal write cycle success
    }
  }
  /****************************read flash data from stm8 end********************************/

  /*****************************read EEPROM data from stm8 start************************************/

  stm8s003_mem_address = STM8_EEPROM_START_ADDRESS;
  at24xx_mem_address = EEPROM_STORE_ADDRESS;

  for (uint8_t i = 0; i < STM8S003_EEPROM_PAGES; i++)
  {
    if (status)
    {
      status = SWIM_ROTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
    }
    if (status)
    {
      //status=AT24CXX_Write_Page(at24xx_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
      //delay_ms(3);// read from stm8 takes 2.9ms // wait for EOP
    }
    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_GREEN_TOGGLE();     // signal write cycle success
    }
  }
  /*****************************read EEPROM data from stm8 end************************************/

  /***************************read Option bytes data from stm8 start*************************/
  if (status)
  {
    status = SWIM_ROTF(SWIM_OPT1, RAM_Buffer, 10); // stm8s003 has 10 option bytes
  }
  if (status)
  {
    //status=AT24CXX_Write_Page(OPTION_BYTE_STORE_ADDRESS, RAM_Buffer, 10);
    delay_ms(3); // // wait for EOP
  }

  return status;
}
/***************************read Option bytes data from stm8 end*************************/
/***** Clone stm8s003  ****/

/****************************** AT24C256_To_STM8 start*******************************************/
uint8_t AT24CXX_To_STM8S003(void)
{
  uint8_t status = 0;
  uint16_t stm8s003_mem_address;
  uint16_t at24xx_mem_address;

  status = SWIM_Enter();

  /********************************write flash data to stm8 start*************************/
  stm8s003_mem_address = STM8_FLASH_START_ADDRESS;
  at24xx_mem_address = FLASH_STORE_ADDRESS;

  if (status)
  {
    status = SWIM_Unlock_Flash();
  }

  for (uint8_t i = 0; i < STM8S003_FLASH_PAGES; i++)
  {
    if (status)
    {
      //status = AT24CXX_Read_Buffer(at24xx_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
    }
    if (status)
    {
      status = SWIM_Enable_Block_Programming(); //standard block programming
    }
    if (status)
    {
      status = SWIM_WOTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
      status = SWIM_Wait_For_EOP();
    }
    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_RED_TOGGLE();
    }
  }

  if (status)
  {
    status = SWIM_Lock_Flash();
  }
  /********************************write flash data to stm8 end*************************/

  /*************************write EEPROM data to stm8 start*********************************/
  stm8s003_mem_address = STM8_EEPROM_START_ADDRESS;
  at24xx_mem_address = EEPROM_STORE_ADDRESS;

  if (status)
  {
    status = SWIM_Unlock_EEPROM();
  }

  for (uint8_t i = 0; i < STM8S003_EEPROM_PAGES; i++)
  {
    if (status)
    {
      //status=AT24CXX_Read_Buffer(at24xx_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
    }
    if (status)
    {
      status = SWIM_Enable_Block_Programming(); //standard block programming
    }
    if (status)
    {
      status = SWIM_WOTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
      status = SWIM_Wait_For_EOP();
    }
    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_RED_TOGGLE();
    }
  }

  if (status)
  {
    //status=SWIM_Lock_EEPROM();
  }
  /*************************write EEPROM data to stm8 end*********************************/

  /************************write Option bytes data to stm8 start******************************/
  if (status)
  {
    status = SWIM_Unlock_OptionByte();
  }
  if (status)
  {
    //status=AT24CXX_Read_Buffer(OPTION_BYTE_STORE_ADDRESS,RAM_Buffer,10);
  }
  if (status)
  {
    status = SWIM_WOTF(SWIM_OPT1, RAM_Buffer, 2);
    status = SWIM_Wait_For_EOP();
  }
  if (status)
  {
    status = SWIM_WOTF(SWIM_OPT2, RAM_Buffer + 2, 2);
    status = SWIM_Wait_For_EOP();
  }
  if (status)
  {
    status = SWIM_WOTF(SWIM_OPT3, RAM_Buffer + 4, 2);
    status = SWIM_Wait_For_EOP();
  }
  if (status)
  {
    status = SWIM_WOTF(SWIM_OPT4, RAM_Buffer + 6, 2);
    status = SWIM_Wait_For_EOP();
  }
  if (status)
  {
    status = SWIM_WOTF(SWIM_OPT5, RAM_Buffer + 8, 2);
    status = SWIM_Wait_For_EOP();
  }
  if (status)
  {
    status = SWIM_Lock_OptionByte();
  }
  /************************write Option bytes data to stm8 end******************************/

  return status;
}
/****************************** AT24C256_To_STM8 end*******************************************/

/***** compare stm8s003  ****/

uint8_t Compare_STM8S003_To_AT24CXX(void)
{
  uint8_t status;
  uint16_t stm8s003_mem_address;
  uint16_t at24xx_mem_address;

  status = SWIM_Enter();

  /****************************compare flash data start********************************/
  stm8s003_mem_address = STM8_FLASH_START_ADDRESS;
  at24xx_mem_address = FLASH_STORE_ADDRESS;

  for (uint8_t i = 0; i < STM8S003_FLASH_PAGES; i++)
  {
    if (status)
    {
      status = AT24CXX_Read_Buffer(at24xx_mem_address, Compare_Buffer, STM8S003_BLOCK_SIZE);
    }

    status = SWIM_ROTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);
    for (uint8_t j = 0; j < STM8S003_BLOCK_SIZE; j++)
    {
      if (RAM_Buffer[j] != Compare_Buffer[j])
      {
        break;
      }
    }

    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_GREEN_TOGGLE(); // signal write cycle success
    }
  }
  /****************************compare flash data end********************************/

  /****************************compare eeprom data start********************************/
  stm8s003_mem_address = STM8_EEPROM_START_ADDRESS;
  at24xx_mem_address = EEPROM_STORE_ADDRESS;

  for (uint8_t i = 0; i < STM8S003_EEPROM_PAGES; i++)
  {
    if (status)
    {
      status = AT24CXX_Read_Buffer(at24xx_mem_address, Compare_Buffer, STM8S003_BLOCK_SIZE);
    }

    status = SWIM_ROTF(stm8s003_mem_address, RAM_Buffer, STM8S003_BLOCK_SIZE);

    for (uint8_t j = 0; j < STM8S003_BLOCK_SIZE; j++)
    {
      if (RAM_Buffer[j] != Compare_Buffer[j])
      {
        break;
      }
    }

    stm8s003_mem_address += STM8S003_BLOCK_SIZE;
    at24xx_mem_address += STM8S003_BLOCK_SIZE;
    if (status)
    {
      //LED_GREEN_TOGGLE(); // signal write cycle success
    }
  }
  /****************************compare eeprom data end********************************/

  /****************************opt eeprom data start********************************/
  if (status)
  {
    status = AT24CXX_Read_Buffer(OPTION_BYTE_STORE_ADDRESS, Compare_Buffer, 10);
  }

  status = SWIM_ROTF(SWIM_OPT1, RAM_Buffer, 10);

  for (uint8_t j = 0; j < STM8S003_BLOCK_SIZE; j++)
  {
    if (RAM_Buffer[j] != Compare_Buffer[j])
    {
      break;
    }

    if (status)
    {
      //LED_GREEN_TOGGLE(); // signal write cycle success
    }
  }
  /****************************compare opt data end********************************/

  return status;
}
/***************************read Option bytes data from stm8 end*************************/
/***** Clone stm8s003  ****/