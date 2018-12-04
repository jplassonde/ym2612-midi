#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "instruments.h"
#include "channel.h"
#include "main.h"

/********* MAIN *********/

int main(void) {

// Init
	int pipe_fd;
	char input_buffer[3];
	CHANNEL * current_chan; 
	INSTRUMENT * current_instrument; 

	CHANNEL channels[CHIP_COUNT*6];

	// Init reset GPIO & set it to low	
	int reset_fd = set_reset();
	
	init_channels(channels);
	
	PATCH_LIST * patch_list;
	create_instruments(&patch_list);	
	current_instrument = &patch_list->instrument[0];	
	
	uint16_t freq_table[128];
	build_freq_table(freq_table);

	CHANQUEUE * chanqueue = create_queue(CHIP_COUNT*6);	

	sleep(1);
	init_i2c();

	write(reset_fd, "1", 1);
	sleep(1);
	
// Initialize chip registers		
	for (int i = 0; i < CHIP_COUNT; i++) {
		write_to_reg(LFO_REG, 0, CS << i, PART_1);
		write_to_reg(CH3_MODE, 0, CS << i, PART_1);
	}
	
	for (int i = 0; i < CHIP_COUNT*6; i++) {
		channels[i].instrument = current_instrument;
		write_instrument(&channels[i]);
		release_chan(chanqueue, &channels[i]); // Store channels pointer in the queue
	}

	pipe_fd = launch_midi();
	
/***************** MAIN LOOP ****************/       
	while(1) {
		while(read(pipe_fd, input_buffer, 3) < 3);
		printf("command:0x%02x, arg1:0x%02x, arg2:0x%02x\n", input_buffer[0], input_buffer[1], input_buffer[2]);
		switch(input_buffer[0]) {
		case NOTE_ON: 
			if (queue_not_empty(chanqueue)) {
				current_chan = get_chan(chanqueue);

				if (current_chan->instrument != current_instrument) {
					current_chan->instrument = current_instrument;
					write_instrument(current_chan);
				}
				current_chan->key = input_buffer[1];
				key_on(current_chan, freq_table[input_buffer[1]], input_buffer[2]);
			}
			break;

		case NOTE_OFF:
			for (int i = 0; i <	CHIP_COUNT*6; i++) {
				if (channels[i].key == input_buffer[1]) {
					key_off(&channels[i]);
					channels[i].key = 0xFF;
					release_chan(chanqueue, &channels[i]);
					break;
				} 
			}
				break;
				
		case SWITCH_INST:
			if (input_buffer[1] < patch_list->count) {
				current_instrument = &patch_list->instrument[input_buffer[1]];
			}
				break;
				
		case ALL_OFF:
			for (int i = 0; i < CHIP_COUNT*6; i++) {
				if (channels[i].key != 0xFF) {
					channels[i].key = 0xFF;
					release_chan(chanqueue, &channels[i]);
				}
			}
			break;
				
		default: 
			printf("Unexpected command: 0x%02x, Exiting.\n", input_buffer[0]);
			exit(1);
		}
	}

	return 0;
}
/******* END MAIN *******/


/*************************************
* Create a child process with a pipe *
*************************************/
int launch_midi() { 
	int pipe_fd[2];
	pid_t pid;
	char fd_str[16];

	pipe(pipe_fd);
	sprintf(fd_str, "%d", pipe_fd[1]);
	pid = fork();

	switch(pid) {
	case -1:
		perror("Process creation failure\n");
		exit(1);
	case 0: // Child process
		close(pipe_fd[0]);
		execl("midiin", "midiin", fd_str, NULL);
	default: // Parent process
		close(pipe_fd[1]);
	}

	return pipe_fd[0];
}



/************************************
* Init reset GPIO and set it to low *
************************************/
int set_reset() {
	int fd;
	// Export GPIO3_4
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		printf("unable to open gpio export\n");
		exit(1);
	}

	write(fd, "68", 2);
	close(fd);

	// Set pin as output
	fd = open("/sys/class/gpio/gpio68/direction", O_WRONLY);
	if (fd < 0) {
		printf("unable to open gpio direction\n");
		exit(1);
	}

	write(fd, "out", 3);
	close(fd);

	// Get file descriptor for GPIO Value
	fd = open("/sys/class/gpio/gpio68/value", O_WRONLY | O_NOCTTY);
	if (fd < 0) {
		printf("unable to open gpio value\n");
		exit(0);
	}
	write(fd, "0", 1); // Set reset low.

	return fd;	
}

void wait_us(int us) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = us * 1000;
	nanosleep(&ts, NULL);
}

/*****************************************************
* Set key on + volume on output operators,           *
* depending of the algorithm (operators arrangement) *
*****************************************************/

void key_on(CHANNEL * chan, uint16_t frequency, uint8_t volume) {
	uint8_t msb = (uint8_t)(frequency >> 8);
	uint8_t lsb = (uint8_t)(frequency & 0xff);
	switch(chan->instrument->algo) {
	case 0:
	case 1:
	case 2:
    		write_to_reg(TL_REG | chan->op4_addr, volume, chan->chip_cs, chan->part);
	break;
	case 4:
    		write_to_reg(TL_REG | chan->op2_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op4_addr, volume, chan->chip_cs, chan->part);
	break;
	case 5:
	case 6:	
    		write_to_reg(TL_REG | chan->op2_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op3_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op4_addr, volume, chan->chip_cs, chan->part);
	break;
	case 7:
    		write_to_reg(TL_REG | chan->op1_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op2_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op3_addr, volume, chan->chip_cs, chan->part);
    		write_to_reg(TL_REG | chan->op4_addr, volume, chan->chip_cs, chan->part);
	break;
	default:
		printf("algo out of bound\n");
	}
	
	write_to_reg(FREQ_MSB | chan->op1_addr, msb, chan->chip_cs, chan->part);
	write_to_reg(FREQ_LSB | chan->op1_addr, lsb, chan->chip_cs, chan->part);
	write_to_reg(KEY_REG, chan->ch_addr | 0xF0, chan->chip_cs, PART_1);
}

void key_off(CHANNEL * chan) {
	write_to_reg(KEY_REG, chan->ch_addr, chan->chip_cs, PART_1);
}

/*****************************************
* write_to_IOX:                          *
* write data to an I/O expander register *
*****************************************/
void write_to_IOX(uint8_t port, uint8_t data) {
	uint8_t i2c_data[] = {port, data};
	write(i2c_fd, i2c_data, 2);
}

/****************************************
* write_to_chip:                        *
* called by write_to_reg				*
* Take care of the YM latching		    * 
****************************************/

void write_to_chip(uint8_t data, uint8_t ctrl, uint8_t cs) {
	write_to_IOX(DATA_PORT, data);
	write_to_IOX(CTRL_PORT, ctrl);
	ctrl &= ~cs;
	write_to_IOX(CTRL_PORT, ctrl);
	wait_us(BUS_PAUSE);
	
	ctrl ^= WR;
	write_to_IOX(CTRL_PORT, ctrl);	
	wait_us(BUS_PAUSE);

	ctrl |= WR;
	write_to_IOX(CTRL_PORT, ctrl);
	wait_us(BUS_PAUSE);
	ctrl |= cs;
	write_to_IOX(CTRL_PORT, ctrl);
	wait_us(BUS_PAUSE);
}

/****************************************
* write_to_reg:                         *
* Select address and set register       *
****************************************/

void write_to_reg(uint8_t reg, uint8_t data, uint8_t chip_cs, uint8_t part) {
#ifndef DEBUG
	uint8_t ctrl = WR | part | CS_MASK;
	write_to_chip(reg, ctrl, chip_cs);
	wait_us(BUS_PAUSE);
	
	ctrl |= DATA_WR;
	write_to_chip(data, ctrl, chip_cs);
	wait_us(WRITE_PAUSE);
#else
	printf("Writing 0x%02x to reg 0x%02x - part %x\n", data, reg, part);
#endif
}

/********************************************************
* set_instrument: 										*
* -Set the YM2612 registers for one channel			    *
********************************************************/
void write_instrument(CHANNEL * chan) {		
	// Set key off before starting -just in case-
	write_to_reg(KEY_REG, chan->ch_addr, chan->chip_cs, PART_1);
	// Set ops params
	set_op(chan->op1_addr, &chan->instrument->op1, chan->chip_cs, chan->part);
    set_op(chan->op2_addr, &chan->instrument->op2, chan->chip_cs, chan->part);
    set_op(chan->op3_addr, &chan->instrument->op3, chan->chip_cs, chan->part);
    set_op(chan->op4_addr, &chan->instrument->op4, chan->chip_cs, chan->part);
	// Set Ch1 Feedback and Algorithm (operators interconnection)
	write_to_reg(FBALG_REG | chan->op1_addr, (chan->instrument->ch1_feedback << 4) | chan->instrument->algo, chan->chip_cs, chan->part);
	// Set pan (Stereo by default), & LFO 
	write_to_reg(PANLFO_REG | chan->op1_addr, (chan->pan << 6) | (chan->instrument->ams) << 4 | chan->instrument->fms, chan->chip_cs, chan->part);
}

/********************************************************
* set_operator:                                         *
* Write the operator registers in the YM2612/3438       *
********************************************************/
void set_op(uint8_t op_addr, OPERATOR * op, uint8_t chip_cs, uint8_t part) {
	write_to_reg(DETMUL_REG | op_addr, (op->dt1 << 4) | op->mul, chip_cs, part);
    write_to_reg(TL_REG | op_addr, op->tl, chip_cs, part);
    write_to_reg(RSAR_REG | op_addr, (op->rs << 6) | op->ar, chip_cs, part);
    write_to_reg(AMD1_REG | op_addr, (op->am << 7) | op->d1r, chip_cs, part);
    write_to_reg(D2R_REG | op_addr, op->d2r, chip_cs, part);
    write_to_reg(D1LRR_REG | op_addr, (op->d1l << 4) | op->rr, chip_cs, part);
    write_to_reg(SSGEG_REG | op_addr, op->ssg_eg, chip_cs, part);
}

/********************************************************
* init_channels:                                        *
* - Initialize 6 channels per Sound Chip 				*
********************************************************/

void init_channels(CHANNEL * channels) {
	for (int j = 0; j < CHIP_COUNT; j++) {
		for (int i = 0; i < 6; i++) {
			channels[i+j].op1_addr = i % 3;
			channels[i+j].op2_addr = (i % 3) + 4;
			channels[i+j].op3_addr = (i % 3) + 8;
			channels[i+j].op4_addr = (i % 3) + 12;
			channels[i+j].ch_addr =  (i < 3 ? i : i+1);
			channels[i+j].part = (i < 3 ? PART_1 : PART_2);
			channels[i+j].chip_cs = CS << j;
			channels[i+j].pan = 0x3;
			channels[i+j].key = 0xff;
		}
	}
}

/*******************************************************************************
* build_freq_table:                                                            *
* - Build the frequency table according to the soundchip(s) clock rate         *
* - Format: 00BBBMMMLLLLLLLL where B = Block; M = MSB; L = LSB                 *
* - Most likely not perfect, as error are accumulating, but gives flexibility  *
*   with the clock and prevents from having to hardcode 128 values             *
*******************************************************************************/

void build_freq_table(uint16_t * table) {
    const float hs_multiplier = 1.05946309436;   // 2^(1/12), multiplier to get the next half-step
    const float sample_frequency = YM_CLOCK / 144;

    float frequency = 8.175;  // first frequency of the table in Hz
    int index = 0;
    uint8_t block = 0;
    uint16_t temp;

    while (index <= 127) {
        // Freq_Hz = Freq_Val * Freq_Sample * 2^block / 2^21
        temp = (uint16_t)(frequency / (sample_frequency * (1 << block) / 2097152));
        if (temp > 2047) {
            ++block;
            continue;
        }


        table[index] = temp | (block << 11);
        frequency = frequency * hs_multiplier;
        ++index;
    }
}

/***************************************************************
* Init I2C, I/O expender and set control port to default value *
***************************************************************/
void init_i2c() {
#ifndef DEBUG
	char msg[2]; // Register + value

	if ((i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0)  {
		perror("Failed to open I2C\n");
		exit(1);
	}

	if (ioctl(i2c_fd, I2C_SLAVE, 0x27) < 0) {
		perror("Failed to set slave\n");
		exit(1);
	}

	// Set both ports as output
	write_to_IOX(IODIRA, 0);
	write_to_IOX(IODIRB, 0);	
	// Init Control Port as 0xFF - All #CS + #WR high 
	write_to_IOX(CTRL_PORT, 0xFF);

#endif
}

