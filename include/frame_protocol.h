#ifndef _FRAME_PROTOCOL_H_
#define _FRAME_PROTOCOL_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /* CAN message object structure */
    typedef struct
    {
        uint32_t id;       /* 29 bit identifier */
        char *data;        /* Data field */
        uint32_t data_len; /* Length of data field in bytes */
        uint8_t ch;        /* Object channel */
        uint8_t format;    /* CAN_FORMAT,0 - STANDARD, 1- EXTENDED IDENTIFIER*/
        uint8_t type;      /* CAN_FRAME,0 - DATA FRAME, 1 - REMOTE FRAME */
        uint32_t total_len;
    } CAN_msg;

#define MAX_CAN_PARSE_BUF_LEN (1024 * 1024)

    enum frame_sync_e
    {
        FS_FIND_AA1,
        FS_FIND_AA2,
        FS_FIND_DATA,
        FS_FIND_CRC, /* 未使用 */
        FS_FIND_551,
        FS_FIND_552
    };

    extern uint8_t g_can_frame[MAX_CAN_PARSE_BUF_LEN];
    extern CAN_msg pmsg;
    extern uint8_t last_byte;
    extern uint32_t frame_idx;
    extern uint32_t total_idx;
    extern enum frame_sync_e sync_state;
    extern unsigned long last_tick;

    inline CAN_msg *uart_rec_canmsg(uint8_t *g_can_frame, uint8_t cur_byte)
    {
        /* 超时复位状态机操作，可根据实际情况删除 */
        // if (GET_DIFF_TICK(last_tick) * 1000 / HZ > 250)
        // {
        //     sync_state = FS_FIND_AA1;
        //     frame_idx = 0;
        // }
        switch (sync_state)
        {
        case FS_FIND_AA1:
            if (cur_byte == 0xAA)
            {
                sync_state = FS_FIND_AA2;
                frame_idx = 0;
                g_can_frame[frame_idx++] = cur_byte;
            }
            break;
        case FS_FIND_AA2:
            if (cur_byte == 0xAA)
            {
                sync_state = FS_FIND_DATA;
                g_can_frame[frame_idx++] = cur_byte;
            }
            else
            {
                sync_state = FS_FIND_AA1;
            }
            break;
        case FS_FIND_DATA:
            total_idx++;
            if ((cur_byte == 0x55 || cur_byte == 0xAA)) //&& frame_idx != (CAN_PKT_SIZE - 3))
            {
                if (last_byte == 0xa5)
                {
                    g_can_frame[frame_idx - 1] = cur_byte;
                }
                else if (last_byte == 0x00)
                {
                    g_can_frame[frame_idx++] = cur_byte;
                    sync_state = FS_FIND_552;
                }
                else
                {
                    sync_state = FS_FIND_AA1;
                    frame_idx = 0;
                    total_idx = 0;
                }
            }
            else
            {
                g_can_frame[frame_idx++] = cur_byte;
            }
            break;
        case FS_FIND_552:
            sync_state = FS_FIND_AA1;
            if (cur_byte == 0x55)
            {
                g_can_frame[frame_idx] = cur_byte;
                pmsg.id = 0;
                for (int i = 0; i < 4; i++)
                    pmsg.id += (g_can_frame[2 + i] << (8 * i));
                pmsg.data = (char *)(&g_can_frame[6]);
                pmsg.data_len = 0;
                for (int i = 0; i < 4; i++)
                    pmsg.data_len += (g_can_frame[frame_idx - 9 + i] << (8 * i));
                /* 存入缓冲区待使用 */
                //memcpy(&g_uart_dat.msg_rcv, pmsg, sizeof(CAN_msg));
                pmsg.total_len = total_idx +3;
                total_idx = 0;
                frame_idx = 0;
                return &pmsg;
            }
            break;
        default:
            break;
        }

        last_byte = cur_byte;
        //last_tick = clock();
        return NULL;
    }

    int pack_send_canbuf(uint32_t id, uint8_t *can_buf, void *buf, uint32_t bufsize);
    inline CAN_msg *uart_rec_canmsg(uint8_t *g_can_frame, uint8_t data);

#ifdef __cplusplus
}
#endif
#endif
