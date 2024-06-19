#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdint.h>
#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "25LC320A.h"

static inline void _eeprom_chip_select(eeprom_t *eeprom) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(eeprom->cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
    /* printf("chip_select is: %d/%d", gpio_get(eeprom->cs_pin), gpio_get(9)); */
}

static inline void _eeprom_chip_deselect(eeprom_t *eeprom) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(eeprom->cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
    /* printf("chip_select is: %d/%d", gpio_get(eeprom->cs_pin), gpio_get(9)); */
}

// firs, check the Write-In_Progress flag of the Read Status Register
// (RDSR) by sending the RDSR instruction and checking the WIP bit (0x01).
// Loop until the WIP flag is not set.
static void _loop_until_WIP_unset(eeprom_t *eeprom) {
    uint8_t wip_buff[2];
    do {
        // things can get a bit yikes at high speeds; the CS line can be low
        // for less than 1us, so the next line smooths things out
        busy_wait_us(1);
        _eeprom_chip_select(eeprom);
        // these need to be reinitialized each loop, or the command flag gets
        // overwritten. Another option is to have a separate buffer for read
        // and write
        wip_buff[0] = EEPROM_RDSR_STATUS_READ;
        wip_buff[1] = 0;
        spi_write_read_blocking(eeprom->spi, wip_buff, wip_buff, 2);
        _eeprom_chip_deselect(eeprom);
    } while (wip_buff[1] & EEPROM_RDSR_WIP_FLAG); 
    return;
} 

extern bool eeprom_init(eeprom_t *eeprom, spi_inst_t *spi, uint8_t cs, uint8_t page) {
    eeprom->spi =           spi;
    eeprom->cs_pin =        cs;
    eeprom->page_size =     page;
    eeprom->last_address =  0x0FFF;
    /* eeprom->current_address = 0; */
    return 0;
} // end eeprom_inti

void eeprom_send_four_bytes(eeprom_t *eeprom, uint16_t address, uint32_t data) {
    _loop_until_WIP_unset(eeprom);
    uint8_t write_buff[4];
    write_buff[0] = EEPROM_WREN_WRITE_ENABLE;
    write_buff[1] = EEPROM_WRITE;
    write_buff[2] = (address >> 8) & 0xFF;
    write_buff[3] = address & 0xFF;

    /* printf("Sending WRITE ENABLE\t"); */
    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, &write_buff[0], 1);
    _eeprom_chip_deselect(eeprom);
    busy_wait_us(1);
    /* if (res != 1) { */
    /*     printf("no result..."); */
    /* } // end if */

    /* printf("Sending WRITE command to [%X%X]\n", write_buff[2], write_buff[3]); */
    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, &write_buff[1], 1);
    spi_write_blocking(eeprom->spi, &write_buff[2], 1);
    spi_write_blocking(eeprom->spi, &write_buff[3], 1);

    write_buff[0] = (data >> 24) & 0xFF;
    write_buff[1] = (data >> 16) & 0xFF;
    write_buff[2] = (data >>  8) & 0xFF;
    write_buff[3] = data & 0x0FFF;
    spi_write_blocking(eeprom->spi, write_buff, 4);
    /* for (uint8_t i = 0; i < 4; i++) { */
    /*     spi_write_blocking(eeprom->spi, &write_buff[i], 1); */
    /* } // end for loop */
    _eeprom_chip_deselect(eeprom);
} //end eepromSendFourBytes

void eeprom_send_one_byte(eeprom_t *eeprom, uint16_t address, uint8_t data) {
    _loop_until_WIP_unset(eeprom);
    uint8_t write_buff[4];
    write_buff[0] = EEPROM_WREN_WRITE_ENABLE;
    write_buff[1] = EEPROM_WRITE;
    write_buff[2] = (address >> 8) & 0xFF;
    write_buff[3] = address & 0xFF;

    /* printf("Sending WRITE ENABLE\t"); */
    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, &write_buff[0], 1);
    _eeprom_chip_deselect(eeprom);
    busy_wait_us(1);
    /* if (res != 1) { */
    /*     printf("no result..."); */
    /* } // end if */

    /* printf("Sending WRITE command to [%X%X]\n", write_buff[2], write_buff[3]); */
    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, &write_buff[1], 1);
    spi_write_blocking(eeprom->spi, &write_buff[2], 1);
    spi_write_blocking(eeprom->spi, &write_buff[3], 1);

    /* write_buff[0] = (data >> 24) & 0xFF; */
    /* write_buff[1] = (data >> 16) & 0xFF; */
    /* write_buff[2] = (data >>  8) & 0xFF; */
    /* write_buff[3] = data & 0x0FFF; */
    spi_write_blocking(eeprom->spi, &data, 1);
    /* for (uint8_t i = 0; i < 4; i++) { */
    /*     spi_write_blocking(eeprom->spi, &write_buff[i], 1); */
    /* } // end for loop */
    _eeprom_chip_deselect(eeprom);
}

uint32_t eeprom_get_four_bytes(eeprom_t *eeprom, uint16_t address) {
    _loop_until_WIP_unset(eeprom);

    uint8_t incoming_data[4] = {0};
    uint8_t write_buff[3];
    write_buff[0] = EEPROM_READ;
    write_buff[1] = (address >> 8) & 0xFF;
    write_buff[2] = address & 0xFF;

    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, write_buff, 3);
    spi_read_blocking(eeprom->spi, 0, incoming_data, 4);
    /* if (res != 4) { */
    /*     printf("no READ result..."); */
    /* } // end if */
    _eeprom_chip_deselect(eeprom);

    uint32_t combined = ((uint32_t)incoming_data[0] << 24) + 
        ((uint32_t)incoming_data[1] << 16) + 
        ((uint32_t)incoming_data[2] << 8) + 
        (uint32_t)incoming_data[3];
    return combined;
}// end  eepromGetFourBytes

uint8_t eeprom_get_one_byte(eeprom_t *eeprom, uint16_t address){
    _loop_until_WIP_unset(eeprom);

    uint8_t incoming_data;
    uint8_t write_buff[3];
    write_buff[0] = EEPROM_READ;
    write_buff[1] = (address >> 8) & 0xFF;
    write_buff[2] = address & 0xFF;

    _eeprom_chip_select(eeprom);
    spi_write_blocking(eeprom->spi, write_buff, 3);
    spi_read_blocking(eeprom->spi, 0, &incoming_data, 1);
    /* if (res != 4) { */
    /*     printf("no READ result..."); */
    /* } // end if */
    _eeprom_chip_deselect(eeprom);

    return incoming_data;
}

void eeprom_dump_all(eeprom_t *eeprom) {
    printf("contents of eeprom:");
    for (int i = 0; i < (4096 / 16); i++) 
    {
        if ((i % 8) == 0) 
        {
            printf("\n\t\t");
            for (int j = 0; j < 16; j++){ printf("%2X ", j); }
            printf("\n\t\t");
            for (int j = 0; j < 16; j++){ printf( "-- "); }
            printf("\n");
        }
        printf("\t%03X_\t", i);
        for (int inner = 0; inner < 16; inner++) 
        {
            uint8_t res = eeprom_get_one_byte(eeprom, ((i << 4) + inner));
            printf("%02x ", res);
        }
        printf("\n");
    }
}

void eeprom_clear_all(eeprom_t *eeprom) 
{
    for (uint16_t address = 0; address < 4096; address++) 
    {
        uint8_t data = 0;
        eeprom_send_one_byte(eeprom, address, data);
    }
}
