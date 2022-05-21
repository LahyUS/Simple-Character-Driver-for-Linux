/*
 * 
 *       
 */

#define REG_SIZE 1         // size of 1 register 1 byte (8 bits)
#define NUM_CTRL_REGS 1    // number of control register
#define NUM_STS_REGS 5     // number of status register
#define NUM_DATA_REGS 256  // number of data register
#define NUM_DEV_REGS (NUM_CTRL_REGS + NUM_STS_REGS + NUM_DATA_REGS) // total register

/****************** Description of Status Register: START ******************/
/*
 * register pair [READ_COUNT_H_REG:READ_COUNT_L_REG]:
 * - Initialized value: 0x0000
 * - increase 1 unit if read successfully
 */
#define READ_COUNT_H_REG 0
#define READ_COUNT_L_REG 1

/*
 * register pair [WRITE_COUNT_H_REG:WRITE_COUNT_L_REG]:
 * - Initialized value: 0x0000
 * - increase 1 unit if write successfully
 */
#define WRITE_COUNT_H_REG 2
#define WRITE_COUNT_L_REG 3

/*
 * register DEVICE_STATUS_REG:
 * - Initialized value: 0x03
 * - Meaning of bits:
 *   bit0:
 *       0: unavailable to read
 *       1: available to read
 *   bit1:
 *       0: unavailable to write
 *       1: available to write
 *   bit2:
 *       0: if data registers were cleared, set bit to 0
 *       1: if all data registers were written, set bit to 1
 *   bit3~7: unused
 */
#define DEVICE_STATUS_REG 4

#define STS_READ_ACCESS_BIT (1 << 0)
#define STS_WRITE_ACCESS_BIT (1 << 1)
#define STS_DATAREGS_OVERFLOW_BIT (1 << 2)

#define READY 1
#define NOT_READY 0
#define OVERFLOW 1
#define NOT_OVERFLOW 0
/****************** Description of Status Register: END ******************/


/****************** Description of Control Register: START ******************/
/*
 * register CONTROL_ACCESS_REG:
 * - role: includes bits that control read/write on data register 
 * - Initialized value: 0x03
 * - Meaning:
 *   bit0:
 *       0: do not allow read from data registers
 *       1: allow read from data registers
 *   bit1:
 *       0: do not allow write on data registers
 *       1: allow write on data registers
 *   bit2~7: unused
 */
#define CONTROL_ACCESS_REG 0

#define CTRL_READ_DATA_BIT (1 << 0)
#define CTRL_WRITE_DATA_BIT (1 << 1)

#define ENABLE 1
#define DISABLE 0
/****************** Description of Control Register: END ******************/
