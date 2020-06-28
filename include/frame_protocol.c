#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <frame_protocol.h>

#define MSG_CH_CAN 0
#define MSG_CH_UART 0x35
/* Symbolic names for formats of CAN message */
typedef enum
{
    STANDARD_FORMAT = 0,
    EXTENDED_FORMAT
} CAN_FORMAT;

/* Symbolic names for type of CAN message */
typedef enum
{
    DATA_FRAME = 0,
    REMOTE_FRAME
} CAN_FRAME;

/* 把发送的数据按照协议打包
 * @bufsize:<=uint32_t
 * @输入: id 发送缓存 数据源地址 数据大小
 * @输出: 打包后的数据大小
 * * 0xaa 0xaa id(4) data len(4) crc 0x55 0x55
 */
int pack_send_canbuf(uint32_t id, uint8_t *can_buf, void *buf, uint32_t bufsize)
{
    int i, idx = 2;
    uint8_t temp, crc = 0, *pbuf = (uint8_t *)buf;
    /* head */
    can_buf[0] = 0xaa;
    can_buf[1] = 0xaa;
    /* ext id */
    id &= 0x3FFFFFFF;
    for (i = 0; i < 4; i++)
    {
        temp = (id >> (i * 8)) & 0xFF;
        if (temp == 0x55 || temp == 0xAA)
        {
            can_buf[idx++] = 0xa5;
        }
        can_buf[idx++] = temp;
        crc += temp;
    }
    /* crc in data[8] */
    for (i = 0; i < bufsize; i++)
    {
        if (pbuf[i] == 0x55 || pbuf[i] == 0xAA)
        {
            can_buf[idx++] = 0xa5;
        }
        can_buf[idx++] = pbuf[i];
        crc += pbuf[i];
    }
    for (; i < 8; i++)
    {
        can_buf[idx++] = 0;
    }
    /* len/formatpe */
    for (i = 0; i < 4; i++)
    {
        temp = (bufsize >> (i * 8)) & 0xFF;
        if (temp == 0x55 || temp == 0xAA)
        {
            can_buf[idx++] = 0xa5;
        }
        can_buf[idx++] = temp;
        crc += temp;
    }
    can_buf[idx++] = MSG_CH_UART; /* ch=0 */              //0x35
    can_buf[idx++] = EXTENDED_FORMAT; /*EXTENDED_FORMAT*/ //1
    can_buf[idx++] = DATA_FRAME;                          //0
    /* crc more */
    for (i = 0; i < 3; i++)
    {
        crc += can_buf[idx - 3 + i];
    }
    crc = 0;
    can_buf[idx++] = crc;
    /* tail */
    can_buf[idx++] = 0x55;
    can_buf[idx++] = 0x55;

    return idx;
}

#define CAN_PKT_SIZE 21
#define GET_DIFF_TICK(tick) ((double)(clock() - (tick)) * 1000 / CLOCKS_PER_SEC)
uint8_t g_can_frame[MAX_CAN_PARSE_BUF_LEN];

/* 从串口接收数据后调用本函数，并组包成can_msg */
CAN_msg pmsg;
/*
* @输入:缓存 字节
* @输出:接收完成返回CAN_msg*,正在接收返回NULL.
*/

uint8_t last_byte = 0;
uint32_t frame_idx = 0;
enum frame_sync_e sync_state = FS_FIND_AA1;
unsigned long last_tick;
uint32_t total_idx = 0;
