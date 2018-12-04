#ifndef _MAIN_G
#define _MAIN_G

/*        Defines        */

#define YM_CLOCK 4000000 // Clock fed to the 2612/3438 in Hertz
#define CHIP_COUNT 1

// Uniques registers
#define LFO_REG 0x22
#define CH3_MODE 0x27
#define KEY_REG 0x28
#define DAC_REG 0x2A
#define DAC_ENR 0x2B

// Channels registers base address (to be ORed with channel/operator offset)
#define STATUS_REG 0x28
#define DETMUL_REG 0x30
#define TL_REG 0x40
#define RSAR_REG 0x50
#define AMD1_REG 0x60
#define D2R_REG 0x70
#define D1LRR_REG 0x80
#define SSGEG_REG 0x90
#define FREQ_LSB 0xA0
#define FREQ_MSB 0xA4
#define FBALG_REG 0xB0
#define PANLFO_REG 0xB4

#define WRITE_PAUSE 0
#define BUS_PAUSE 0

// I/O Expander registers
#define IODIRA      0x00
#define IODIRB      0x01
#define DATA_PORT   0x12
#define CTRL_PORT   0x13

// I/O Expander control pins
#define WR      1
#define PART_1  0 
#define PART_2  4
#define DATA_WR 2
#define CS      8
#define CS_MASK 0xF8

// Controls from midi in process
#define NOTE_ON  0x00
#define NOTE_OFF 0x01
#define SWITCH_INST 0x02
#define ALL_OFF 0x03

/*     Function prototypes     */

int launch_midi();
int set_reset();
void init_i2c(void);
void wait_us(int us);

void key_on(CHANNEL * chan, uint16_t frequency, uint8_t volume);
void key_off(CHANNEL * chan);

void write_to_IOX(uint8_t reg, uint8_t data);
void send_to_2612(uint8_t data, uint8_t reg);
void write_to_reg(uint8_t reg, uint8_t data, uint8_t chip_cs, uint8_t part);

void write_instrument(CHANNEL * chan);
void set_op(uint8_t op_addr, OPERATOR * op, uint8_t chip_cs, uint8_t part);

void build_freq_table(uint16_t * table);
void init_channels(CHANNEL * channels);

int i2c_fd;

#endif
