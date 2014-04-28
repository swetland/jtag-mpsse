#ifndef _I2C_H_
#define _I2C_H_

void i2c_init(void);
int i2c_wr16(unsigned saddr, unsigned addr, unsigned val);
int i2c_rd16(unsigned saddr, unsigned addr, unsigned *val);

#endif
