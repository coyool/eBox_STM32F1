#include "sx1278.h"

void Lora::begin(uint8_t dev_num,uint8_t bw, uint8_t cr, uint8_t sf)
{
    spi_config.dev_num = dev_num;
    spi_config.mode = SPI_MODE0;
    spi_config.prescaler = SPI_CLOCK_DIV32;
    spi_config.bit_order = SPI_BITODER_MSB;
    
    cs->mode(OUTPUT_PP);
    rst_pin->mode(OUTPUT_PP);

    cs->set();
    rst_pin->reset();
    delay_ms(100);
    rst_pin->set();
    spi->begin(&spi_config);
    delay_ms(100);
    config(bw,cr,sf);
    delay_ms(100);
}
void Lora::config(uint8_t bw, uint8_t cr, uint8_t sf) {
    //1,��λ���Ÿ�λ
    //2.��ʱ100ms
    //3.����sleepmode
    //4.�����ⲿ����
    //5.����loraģʽ
    //6.�����ز�Ƶ��
    setMode(SX1278_SLEEP);
    uart1.printf("SX1278_REG_OP_MODE:0x%x\n",readRegister(SX1278_REG_OP_MODE));
    setRegValue(SX1278_REG_OP_MODE, SX1278_LORA, 7, 7);//����loraģʽ
    uart1.printf("SX1278_REG_OP_MODE:0x%x\n",readRegister(SX1278_REG_OP_MODE));

    //6.�����ز�Ƶ��
    setRegValue(SX1278_REG_FRF_MSB, SX1278_FRF_MSB);
    setRegValue(SX1278_REG_FRF_MID, SX1278_FRF_MID);
    setRegValue(SX1278_REG_FRF_LSB, SX1278_FRF_LSB);
    uart1.printf("SX1278_REG_FRF_MSB:0x%x\n",readRegister(SX1278_REG_FRF_MSB));
    uart1.printf("SX1278_REG_FRF_MID:0x%x\n",readRegister(SX1278_REG_FRF_MID));
    uart1.printf("SX1278_REG_FRF_LSB:0x%x\n",readRegister(SX1278_REG_FRF_LSB));

    //7�������������    
    setRegValue(SX1278_REG_PA_CONFIG, SX1278_PA_SELECT_BOOST | SX1278_MAX_POWER | SX1278_OUTPUT_POWER);
    uart1.printf("SX1278_REG_PA_CONFIG:0x%x\n",readRegister(SX1278_REG_PA_CONFIG));

    //8������PA�Ĺ����������رգ�������΢��Ĭ��0x0b
    setRegValue(SX1278_REG_OCP, SX1278_OCP_OFF | SX1278_OCP_TRIM, 5, 0);
    uart1.printf("SX1278_REG_OCP:0x%x\n",readRegister(SX1278_REG_OCP));


    //9.LNA ��������001���������
    setRegValue(SX1278_REG_LNA, SX1278_LNA_GAIN_1 | SX1278_LNA_BOOST_HF_ON);

    if(sf == SX1278_SF_6) {
        setRegValue(SX1278_REG_MODEM_CONFIG_1, bw | cr | SX1278_HEADER_IMPL_MODE);
        setRegValue(SX1278_REG_MODEM_CONFIG_2, SX1278_SF_6 | SX1278_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_ON | SX1278_RX_TIMEOUT_MSB);
        //loraģʽ����Ч
//        setRegValue(SX1278_REG_DETECT_OPTIMIZE, SX1278_DETECT_OPTIMIZE_SF_6, 2, 0);
//        setRegValue(SX1278_REG_DETECTION_THRESHOLD, SX1278_DETECTION_THRESHOLD_SF_6);
    } else {
        setRegValue(SX1278_REG_MODEM_CONFIG_2, sf | SX1278_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_ON | SX1278_RX_TIMEOUT_MSB);
        setRegValue(SX1278_REG_MODEM_CONFIG_1, bw | cr | SX1278_HEADER_EXPL_MODE);
        //loraģʽ����Ч
//        setRegValue(SX1278_REG_DETECT_OPTIMIZE, SX1278_DETECT_OPTIMIZE_SF_7_12, 2, 0);
//        setRegValue(SX1278_REG_DETECTION_THRESHOLD, SX1278_DETECTION_THRESHOLD_SF_7_12);
    }

    setRegValue(SX1278_REG_SYMB_TIMEOUT_LSB, 0x0f);
    
    setRegValue(SX1278_REG_PREAMBLE_MSB, 0);
    setRegValue(SX1278_REG_PREAMBLE_LSB, 16);

    setMode(SX1278_STANDBY);
}

void Lora::enttry_tx() {
    uart1.printf("MODE:0x%x\n",readRegister(SX1278_REG_OP_MODE));
  setMode(SX1278_STANDBY);
    uart1.printf("MODE:0x%x\n",readRegister(SX1278_REG_OP_MODE));
  setRegValue(SX1278_REG_PA_DAC, SX1278_PA_BOOST_ON, 2, 0);
  setRegValue(SX1278_REG_HOP_PERIOD, SX1278_HOP_PERIOD_OFF);
  setRegValue(SX1278_REG_DIO_MAPPING_1, SX1278_DIO0_TX_DONE, 7, 6);
  clearIRQFlags();
  setRegValue(SX1278_REG_IRQ_FLAGS_MASK, SX1278_MASK_IRQ_FLAG_TX_DONE);
  state = TXREADY;
}
void Lora::tx_packet(packet* pack)
{
  uint8_t packetLength = getPacketLength(pack);
  if(packetLength > 255) {
    return ;
  }
  setRegValue(SX1278_REG_PAYLOAD_LENGTH, packetLength);
  setRegValue(SX1278_REG_FIFO_TX_BASE_ADDR, SX1278_FIFO_TX_BASE_ADDR_MAX);
  setRegValue(SX1278_REG_FIFO_ADDR_PTR, SX1278_FIFO_TX_BASE_ADDR_MAX);
  
  writeRegisterBurst(SX1278_REG_FIFO, pack->source, 8);
  writeRegisterBurst(SX1278_REG_FIFO, pack->destination, 8);
  writeRegisterBurst(SX1278_REG_FIFO, pack->data, packetLength - 16);
  
  setMode(SX1278_TX);
  state = TXING;
}

void Lora::tc_evnet()
{
    state=TXREADY;
  clearIRQFlags();  
}

void Lora::enttry_rx() {
    setMode(SX1278_STANDBY);
    setRegValue(SX1278_REG_PA_DAC, SX1278_PA_BOOST_OFF);
    setRegValue(SX1278_REG_HOP_PERIOD, SX1278_HOP_PERIOD_MAX);
    setRegValue(SX1278_REG_DIO_MAPPING_1, SX1278_DIO0_RX_DONE | SX1278_DIO1_RX_TIMEOUT, 7, 5);
    setRegValue(SX1278_REG_IRQ_FLAGS_MASK, (SX1278_MASK_IRQ_FLAG_RX_TIMEOUT & SX1278_MASK_IRQ_FLAG_RX_DONE));
    clearIRQFlags();
    
    setRegValue(SX1278_REG_FIFO_RX_BASE_ADDR, SX1278_FIFO_RX_BASE_ADDR_MAX);
    setRegValue(SX1278_REG_FIFO_ADDR_PTR, SX1278_FIFO_RX_BASE_ADDR_MAX);
    setMode(SX1278_RXCONTINUOUS); 
}
void Lora::rx_packet(packet* p)
{
    uint8_t packetLength;
    uint8_t sf = getRegValue(SX1278_REG_MODEM_CONFIG_2, 7, 4) & B00001111;
    if(sf != SX1278_SF_6) {
      packetLength = getRegValue(SX1278_REG_RX_NB_BYTES);
    }
    else
    {
        packetLength = 21;
    }
    
    for(uint8_t i = 0; i < 8; i++) {
      p->source[i] = readRegister(SX1278_REG_FIFO);
    }
    for(uint8_t i = 0; i < 8; i++) {
      p->destination[i] = readRegister(SX1278_REG_FIFO);
    }
    readRegisterBurst(SX1278_REG_FIFO, p->data,packetLength - 16);
    uart1.printf("SX1278_REG_FIFO_RX_CURRENT_ADDR:0x%x\n",readRegister(SX1278_REG_FIFO_RX_CURRENT_ADDR));

    //���³�ʼ��FIFO��ַ
//    setRegValue(SX1278_REG_FIFO_RX_BASE_ADDR, SX1278_FIFO_RX_BASE_ADDR_MAX);
//    setRegValue(SX1278_REG_FIFO_ADDR_PTR, SX1278_FIFO_RX_BASE_ADDR_MAX);
    //����жϱ�־λ
    clearIRQFlags();
}

void Lora::setMode(uint8_t mode) {
  setRegValue(SX1278_REG_OP_MODE, mode, 2, 0);
}
void Lora::setPacketSource(packet* pack, uint8_t* address) {
  for(uint8_t i = 0; i < 8; i++) {
    pack->source[i] = address[i];
  }
}
void Lora::setPacketDestination(packet* pack, uint8_t* address) {
  for(uint8_t i = 0; i < 8; i++) {
    pack->destination[i] = address[i];
  }
}
void Lora::setPacketData(packet* pack, const char* data) {
  pack->data = (uint8_t *)data;
}



uint8_t Lora::setRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7)) {
    return 0xFF;
  }
  uint8_t currentValue = readRegister(reg);
  uint8_t newValue = currentValue & ((0xff << (msb + 1)) | (0xff >> (8 - lsb)));
  writeRegister(reg, newValue | value);
  return 0;
}

uint8_t Lora::getRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7)) {
    return 0xFF;
  }
  uint8_t rawValue = readRegister(reg);
  uint8_t maskedValue = rawValue & ((0xff << lsb) | (0xff >> (7 - msb)));
  return(maskedValue);
}

void Lora::clearIRQFlags(void) {
  writeRegister(SX1278_REG_IRQ_FLAGS, B11111111);
}

void Lora::readRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes){
  spi->take_spi_right(&spi_config);
  cs->reset();
  spi->transfer(reg | SX1278_READ);
  for(uint8_t i = 0; i< numBytes; i++) {
    data[i] = spi->transfer(reg);
  }
  cs->set();
  spi->release_spi_right();
}

void Lora::writeRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes) {
  spi->take_spi_right(&spi_config);
  cs->reset();
  spi->transfer(reg | SX1278_WRITE);
  for(uint8_t i = 0; i< numBytes; i++) {
    spi->transfer(data[i]);
  }
  cs->set();
  spi->release_spi_right();
}


uint8_t Lora::readRegister(uint8_t reg) {
    uint8_t inByte;
    spi->take_spi_right(&spi_config);
    cs->reset();
    spi->transfer(reg | SX1278_READ);
    inByte = spi->transfer(0xff);
    cs->set();
    spi->release_spi_right();
    return inByte;
}
void Lora::writeRegister(uint8_t reg, uint8_t data) {
    spi->take_spi_right(&spi_config);
    cs->reset();
    spi->transfer(reg | SX1278_WRITE);
    spi->transfer(data);
    cs->set();
    spi->release_spi_right();    
}

void Lora::generateLoRaAdress(void) {
  for(uint8_t i = 0; i < 8; i++) {
   // EEPROM.write(i, (uint8_t)random(0, 256));
  }
}
uint8_t Lora::getPacketLength(packet* pack) {
  for(uint8_t i = 0; i < 240; i++) {
    if(pack->data[i] == '\0') {
      return(i + 17);
    }
  }
}