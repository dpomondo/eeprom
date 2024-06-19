/* for talking to 25xx320 EEPROM
 *
 *                    +---\/---+
 *               _CS  |1*     8|  VCC
 *               SO   |2      7|  _HOLD
 *               _WP  |3      6|  SCK
 *           VSS/GND  |4      5|  SI
 *                    +--------+
 *
 * Project header file requires the following defines:
 *
#define SPI_PORT                    spi_inst_t *spi, 
#define EEPROM_CS_PIN               [see Pico datasheet]
#define EEPROM_CLK_PIN              [*]
#define EEPROM_MOSI_PIN             [*]
#define EEPROM_MISO_PIN             [*]

#define EEPROM_BYTES_PER_PAGE
 *
 * connect:
 *      eeprom:                 pico:
 *      ------                  ----
 *      pin 1 _CS               EEPROM_CS_PIN
 *      pin 2 SO                EEPROM_MISO_PIN         
 *      pin 5 SI                EEPROM_MOSI_PIN         
 *      pin 6 SCK               EEPROM_CLK_PIN          
 *      pins 3, 7, 8            VCC
 *      pin 4                   GND
 *  
 *  Initiate SPI with:
 *
spi_init(SPI_PORT, uint baudrate);
spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
gpio_set_function(EEPROM_CLK_PIN,  GPIO_FUNC_SPI);
gpio_set_function(EEPROM_MOSI_PIN, GPIO_FUNC_SPI);
gpio_set_function(EEPROM_MISO_PIN, GPIO_FUNC_SPI);

 * Create & initialize eeprom struct instance:
eeprom_t eeprom;
eeprom_init(&eeprom, SPI_PORT, EEPROM_CS_PIN, EEPROM_BYTES_PER_PAGE);
 * 
 * initialize eeprom CS pin:
gpio_init(eeprom.cs_pin);
gpio_set_dir(eeprom.cs_pin, GPIO_OUT);
gpio_put(eeprom.cs_pin, 1);
 *
 * */

#ifndef EEPROM_25XX320_h
#define EEPROM_25XX320_h

#include "hardware/spi.h"
#include <stdint.h>
#define EEPROM_READ                 0b00000011
#define EEPROM_WRITE                0b00000010
#define EEPROM_WRDI_WRITE_DISABLE   0b00000100
#define EEPROM_WREN_WRITE_ENABLE    0b00000110
#define EEPROM_RDSR_STATUS_READ     0b00000101
#define EEPROM_WRSR_STATUS_WRITE    0b00000001
#define EEPROM_RDSR_WIP_FLAG        0b00000001

#define EEPROM_WRITE_IN_PROGRESS               0       // write in progess READONLY
#define EEPROM_WRITE_ENABLE_LATCH              1       // write-enable latch READONLY
#define EEPROM_BLOCK_PROTECT_0                 2       // block protection bit 0 W/R
#define EEPROM_BLOCK_PROTECT_1                 3       // block protection bit 1 W/R

#define EEPROM_LAST_ADDRESS         0x0FFF


/* initialize the spi
 * */
typedef struct {
    spi_inst_t  *spi;
    uint8_t     cs_pin;
    uint8_t     page_size;
    uint16_t    last_address;
    uint16_t    current_address; 
} eeprom_t;

/*
 *  @param eeprom:      pointer to eeprom_t struct
 *  @param *spi:        point to spi instance talking to this eeprom
 *  @param cs:          gpio pin used as Chip Select
 *  @param page:        page size for eeprom (probably 32)
 * */
extern bool eeprom_init(eeprom_t *eeprom, spi_inst_t *spi, uint8_t cs, uint8_t page);

/* 25xx320 write sequence (datasheet page 8):
 *      set Write Enable latch: 1. set CS low 
 *                              2. send WREN (write enable) instruction
 *                              3. set CS high
 *      write data:             4. set CS low
 *                              5. send Write instruction
 *                              6. send 16-bit address (4 high bits are ignored)
 *                              7. send up to 32 bytes of data
 *                              8 set CS high
 *      note: writes past a page boundary will wrap around, overwriting the
 *      start of the page. THis means maximum amount that can be written is:
 *
 *      eeprom_page_size - (address % eeprom_page_size)
 *
 *      So if the address is an integer multiple of eeprom_page_size then 32
 *      bytes can be written before wrapping around. 
 * */
extern void eeprom_send_four_bytes(eeprom_t *eeprom, uint16_t address, uint32_t data);

extern void eeprom_send_one_byte(eeprom_t *eeprom, uint16_t address, uint8_t data);

/* 25xx320 read sequence (datasheet page 8):
 *     pre: check Read Status Register for Write-In_Progress bit
 *          (page 11 of datasheet)
 *       1. set CS low 
 *       2. send Read instruction
 *       3. send 16-bit address (4 high bits are ignored)
 *       4. read data -- an arbitrary number of bytes can be read, with the
 *          self-incrementing addresses rolling over to 0 
 *       5. set CS high
* */
extern uint32_t eeprom_get_four_bytes(eeprom_t *eeprom, uint16_t address);

extern uint8_t eeprom_get_one_byte(eeprom_t *eeprom, uint16_t address);

extern void eeprom_dump_all(eeprom_t *eeprom);

extern void eeprom_clear_all(eeprom_t *eeprom) ;

/*
extern void eepromWriteLoops(void);
extern void eepromWriteHighTEmp(void);
extern void eepromWriteLowTEmp(void);

extern uint32_t eepromGetLoops();
extern uint32_t eepromGetHighTEmp();
extern uint32_t eepromGetLowTemp();
*/

#endif // !EEPROM_25XX320_h
