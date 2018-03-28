#define APB_CLOCK 250000000

#define DEN_PIN   18
#define CS_M_PIN  19
#define CS_AG_PIN 20

#define ROUND_DIVISION(x,y) (((x) + (y)/2)/(y))

volatile struct io_peripherals *io;

struct pcm_register
{
    ; /* empty structure */
};

struct io_peripherals
{
  uint8_t               unused0[0x200000];
  struct gpio_register  gpio;             /* offset = 0x200000, width = 0x84 */
  uint8_t               unused1[0x3000-sizeof(struct gpio_register)];
  struct pcm_register   pcm;              /* offset = 0x203000, width = 0x24 */
  uint8_t               unused2[0x1000-sizeof(struct pcm_register)];
  struct spi_register   spi;              /* offset = 0x204000, width = 0x18 */
  uint8_t               unused3[0x8000-sizeof(struct spi_register)];
  struct pwm_register   pwm;              /* offset = 0x20c000, width = 0x28 */
};

struct XYZ {
	float x;
	float y;
	float z;
};

struct XYZ read_accelerometer(volatile struct spi_register *spi, volatile struct gpio_register*gpio);
struct XYZ read_gyroscope(volatile struct spi_register *spi, volatile struct gpio_register*gpio);
struct XYZ read_magnetometer(volatile struct spi_register *spi, volatile struct gpio_register*gpio);
int initialize_accelerometer_project();


