/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define SRAM_SPEED  16000000; 
#define FRAM_SPEED  20000000; 
#define CBRAM_SPEED 1000000; 
#define ReRAM_SPEED 5000000; 

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint32_t mode;
static uint8_t bits = 8;
static char *input_file;
static char *output_file;
static uint32_t speed;
static uint16_t delay;
static int verbose;

enum mem_type  { SRAM = 1 , FRAM = 2 , CBRAM = 3, ReRAM = 4 } ;
enum mem_type mem_mode; //memory chip mode  

uint8_t SEQWR_SRAM_tx[] = {
	0x01, 0x04, 0x02, 0x00,
};
uint8_t SEQRD_SRAM_tx[] = {
	0x01, 0x04, 0x03, 0x00,
};

uint8_t SEQWR_FRAM_tx[] = {
	0x06, 0x02, 0x00, 0xFF, 0x03, 0x00,
};
uint8_t SEQRD_FRAM_tx[] = {
	0x05,// 0x00,// 0x03, 0x01,
};

//uint8_t SEQWR_CBRAM_tx[] = {
	//0x01, 0x04, 0x02, 0x00,
//};
//uint8_t SEQRD_CBRAM_tx[] = {
	//0x1, 0x04, 0x03, 0x00,
//};

//uint8_t SEQWR_ReRAM_tx[] = {
	//0x01, 0x04, 0x02, 0x00,
//};
//uint8_t SEQRD_ReRAM_tx[] = {
	//0x1, 0x04, 0x03, 0x00,
//};

uint8_t default_rx[ARRAY_SIZE(SEQWR_SRAM_tx)] = {0, };
char *input_tx;


//function for printing arrays
static void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D 		--device   device to use (default /dev/spidev0.0)\n"
	     "  -s 		--speed    max speed (Hz)\n"
	     "  -d 		--delay    delay (usec)\n"
	     "  -b 		--bpw      bits per word\n"
	     "  -i		--input    input data from a file (e.g. \"test.bin\")\n"
	     "  -o 		--output   output data to a file (e.g. \"results.bin\")\n"
	     "  -l 		--loop     loopback\n"
	     "  -H 		--cpha     clock phase\n"
	     "  -O 		--cpol     clock polarity\n"
	     "  -L 		--lsb      least significant bit first\n"
	     "  -c 		--cs-high  chip select active high\n"
	     "  -3 		--3wire    SI/SO signals shared\n"
	     "  -v 		--verbose  Verbose (show tx buffer)\n"
	     "  -p      --Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -N 		--no-cs    no chip select\n"
	     "  -r 		--ready    slave pulls low to pause\n"
	     "  -2 		--dual     dual transfer\n"
	     "  -4 		--quad     quad transfer\n"
	     "  -SRAM	--SRAM     SRAM Memory chip\n"
	     "  -FRAM	--FRAM     FRAM Memory chip\n"
	     "  -CBRAM  --CBRAM    CBRAM Memory chip\n"
	     "  -ReRAM 	--ReRAM    ReRAM Memory chip\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "input",   1, 0, 'i' },
			{ "output",  1, 0, 'o' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'c' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'r' },
			{ "dual",    0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			{ "SRAM",    1, 0, 'S' },
			{ "FRAM",    1, 0, 'F' },
			{ "CBRAM",   1, 0, 'C' },
			{ "ReRAM",   1, 0, 'R' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:i:o:S:F:C:R:lHOLC3NR24p:v",
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'c':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'r':
			mode |= SPI_READY;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
		case 'S':
			mem_mode = SRAM;
			break;
		case 'F':
			mem_mode = FRAM;
			break;
		case 'C':
			mem_mode = CBRAM;
			break;
		case 'R':
			mem_mode = ReRAM;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
	if (mode & SPI_LOOP) {
		if (mode & SPI_TX_DUAL)
			mode |= SPI_RX_DUAL;
		if (mode & SPI_TX_QUAD)
			mode |= SPI_RX_QUAD;
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	int match;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}
//SPI master transfer
static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	int out_fd;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose)
		hex_dump(tx, len, 32, "TX");

	if (output_file) {
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");

		ret = write(out_fd, rx, len);
		if (ret != len)
			pabort("not all bytes written to output file");

		close(out_fd);
	}

	if (verbose || !output_file)
		hex_dump(rx, len, 32, "RX");
}


static void transfer_escaped_string(int fd, char *str)
{
	size_t size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(fd, tx, rx, size);
	free(rx);
	free(tx);
}

static void transfer_file(int fd, char *filename)
{
	int i=0,j=0;
	ssize_t bytes;
	struct stat sb;
	int tx_fd;
	uint8_t *tx;
	uint8_t *tx_ready;
	uint8_t *tx_WR;
	uint8_t *rx;

	if (stat(filename, &sb) == -1)
		pabort("can't stat input file");

	tx_fd = open(filename, O_RDONLY);
	if (tx_fd < 0)
		pabort("can't open input file");

	tx = malloc(sb.st_size);
	if (!tx)
		pabort("can't allocate tx buffer");
		
	tx_ready = malloc(2 * sb.st_size);
	if (!tx_ready)
		pabort("can't allocate tx buffer");
		
	tx_WR = malloc(2 * sb.st_size);
	if (!tx_WR)
		pabort("can't allocate tx buffer");
		
	bytes = read(tx_fd, tx, sb.st_size);
	if (bytes != sb.st_size)
		pabort("failed to read input file");
		
	while (tx[i]!='\0'){
		if (tx[i]==' ' ){
			tx_ready[2*i] = tx[i];
			tx_ready[2*i+1] = tx[i];
			//printf("%c-%c%c+",tx[i],tx_ready[2*i],tx_ready[2*i+1]);
		}
		else{
			tx_ready[2*i] = SEQWR_FRAM_tx[2] ;
			tx_ready[2*i+1] = tx[i];
			//printf("%c-%d%c+",tx[i],tx_ready[2*i],tx_ready[2*i+1]);
		}
		i++;	
	}
	//tx_ready[2*i]='\0';
	i=0;
	while (tx_ready[i]!='\0'){
		if ((tx_ready[i]==' ' && tx_ready[i+1]==' ') || (tx_ready[i]=='\n' && tx_ready[i+1]=='\n')){
			i++;
		}
		tx_WR[j] = tx_ready[i];
		j++;
		i++;	
	}
	//tx[j]='\0';
	int len_tx_file=j;
	
	rx = malloc(2 * sb.st_size);
	if (!rx)
		pabort("can't allocate rx buffer");

	transfer(fd, tx_WR, rx, len_tx_file);
	free(rx);
	free(tx_WR);
	free(tx);
	free(tx_ready);
	close(tx_fd);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	parse_opts(argc, argv);

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);

	if (input_tx && input_file)
		pabort("only one of -p and --input may be selected");

	
	switch (mem_mode) {
	case SRAM:
		printf("memory chip: SRAM\n");
		speed = SRAM_SPEED;
		break;
	case FRAM:
		printf("memory chip: FRAM\n");
		speed = FRAM_SPEED;
		break;
	case CBRAM :
		printf("memory chip: CBRAM\n");
		speed = CBRAM_SPEED;
		break;
	case ReRAM:
		printf("memory chip: ReRAM\n");
		speed = ReRAM_SPEED;
		break;
	};
	printf("max speed: %d Hz (%d MHz)\n", speed, speed/1000000);
	

	// SEQUENTIAL WRITE SEQUENCE
	transfer(fd, SEQWR_FRAM_tx, default_rx, sizeof(SEQWR_FRAM_tx));
	
	if (input_tx){
		transfer_escaped_string(fd, input_tx);
	}
	else if (input_file){
		transfer_file(fd, input_file);
	}
	else{
		//transfer(fd, default_tx, default_rx, sizeof(default_tx));
	}
	
	// SEQUENTIAL READ SEQUENCE
	//transfer(fd, SEQRD_FRAM_tx, default_rx, sizeof(SEQRD_FRAM_tx));
	
	close(fd);

	return ret;
}
