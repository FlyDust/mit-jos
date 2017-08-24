 //transmit descriptor
 // 63               48 47    40 39     32 31     24 23     16 15                 0
 // +--------------------------------------------------------------------------------+
 // |                                        Buffer address                                     |
 // +----------------+---------+----------+----------+---------+-----------------+
 // |    Special    |  CSS  | Status|  Cmd  |  CSO  |    Length    |
 // +----------------+---------+----------+----------+---------+-----------------+

struct tx_desc{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};
// 63         48 47      40 39       32 31                                                 16 15             0
// +------------------------------------------------------------------------------------------------+
// |                                     Buffer Address [63:0]                                                  |
// +------------+----------+-----------+--------------------------------------------+------------+
// | Special | Errors | Status | Packet Checksum(See Note)  | Length |
// +------------+----------+-----------+--------------------------------------------+------------+
/* Receive Descriptor */
struct rx_desc {
    uint64_t addr; /* Address of the descriptor's data buffer */
    uint16_t length;     /* Length of data DMAed into data buffer */
    uint16_t csum;       /* Packet checksum */
    uint8_t status;      /* Descriptor status */
    uint8_t errors;      /* Descriptor Errors */
    uint16_t special;
};
