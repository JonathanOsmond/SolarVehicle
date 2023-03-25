#include "CAN.h"
//For CAN1 filter
void CAN_wrFilter (uint32_t id)  {
    static int CAN_std_cnt = 0;
    uint32_t buf0, buf1;
    int cnt1, cnt2, bound1;
 
    /* Acceptance Filter Memory full */
    if (((CAN_std_cnt + 1) >> 1) >= 512)
        return;                                       /* error: objects full */
 
    /* Setup Acceptance Filter Configuration
      Acceptance Filter Mode Register = Off  */
    LPC_CANAF->AFMR = 0x00000001;
 
//    id |= 1 << 13;                        /* Add controller number(2) */
    id &= 0x0000F7FF;                            /* Mask out 16-bits of ID */
 
    if (CAN_std_cnt == 0)  {                     /* For entering first  ID */
        LPC_CANAF_RAM->mask[0] = 0x0000FFFF | (id << 16);
    }  else if (CAN_std_cnt == 1)  {             /* For entering second ID */
        if ((LPC_CANAF_RAM->mask[0] >> 16) > id)
            LPC_CANAF_RAM->mask[0] = (LPC_CANAF_RAM->mask[0] >> 16) | (id << 16);
        else
            LPC_CANAF_RAM->mask[0] = (LPC_CANAF_RAM->mask[0] & 0xFFFF0000) | id;
    }  else  {
        /* Find where to insert new ID */
        cnt1 = 0;
        cnt2 = CAN_std_cnt;
        bound1 = (CAN_std_cnt - 1) >> 1;
        while (cnt1 <= bound1)  {                  /* Loop through standard existing IDs */
            if ((LPC_CANAF_RAM->mask[cnt1] >> 16) > id)  {
                cnt2 = cnt1 * 2;
                break;
            }
            if ((LPC_CANAF_RAM->mask[cnt1] & 0x0000FFFF) > id)  {
                cnt2 = cnt1 * 2 + 1;
                break;
            }
            cnt1++;                                  /* cnt1 = U32 where to insert new ID */
        }                                          /* cnt2 = U16 where to insert new ID */
 
        if (cnt1 > bound1)  {                      /* Adding ID as last entry */
            if ((CAN_std_cnt & 0x0001) == 0)         /* Even number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1]  = 0x0000FFFF | (id << 16);
            else                                     /* Odd  number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1]  = (LPC_CANAF_RAM->mask[cnt1] & 0xFFFF0000) | id;
        }  else  {
            buf0 = LPC_CANAF_RAM->mask[cnt1];        /* Remember current entry */
            if ((cnt2 & 0x0001) == 0)                /* Insert new mask to even address */
                buf1 = (id << 16) | (buf0 >> 16);
            else                                     /* Insert new mask to odd  address */
                buf1 = (buf0 & 0xFFFF0000) | id;
 
            LPC_CANAF_RAM->mask[cnt1] = buf1;        /* Insert mask */
 
            bound1 = CAN_std_cnt >> 1;
            /* Move all remaining standard mask entries one place up */
            while (cnt1 < bound1)  {
                cnt1++;
                buf1  = LPC_CANAF_RAM->mask[cnt1];
                LPC_CANAF_RAM->mask[cnt1] = (buf1 >> 16) | (buf0 << 16);
                buf0  = buf1;
            }
 
            if ((CAN_std_cnt & 0x0001) == 0)         /* Even number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1] = (LPC_CANAF_RAM->mask[cnt1] & 0xFFFF0000) | (0x0000FFFF);
        }
    }
    CAN_std_cnt++;
 
    /* Calculate std ID start address (buf0) and ext ID start address <- none (buf1) */
    buf0 = ((CAN_std_cnt + 1) >> 1) << 2;
    buf1 = buf0;
 
    /* Setup acceptance filter pointers */
    LPC_CANAF->SFF_sa     = 0;
    LPC_CANAF->SFF_GRP_sa = buf0;
    LPC_CANAF->EFF_sa     = buf0;
    LPC_CANAF->EFF_GRP_sa = buf1;
    LPC_CANAF->ENDofTable = buf1;
 
    LPC_CANAF->AFMR = 0x00000000;                  /* Use acceptance filter */
} 

// CAN2_wrFilter
void CAN2_wrFilter (uint32_t id)  {
    static int CAN_std_cnt = 0;
    uint32_t buf0, buf1;
    int cnt1, cnt2, bound1;
 
    /* Acceptance Filter Memory full */
    if (((CAN_std_cnt + 1) >> 1) >= 512)
        return;                                       /* error: objects full */
 
    /* Setup Acceptance Filter Configuration
      Acceptance Filter Mode Register = Off  */
    LPC_CANAF->AFMR = 0x00000001;
 
    id |= 1 << 13;                        /* Add controller number(2) */
    id &= 0x0000F7FF;                            /* Mask out 16-bits of ID */
 
    if (CAN_std_cnt == 0)  {                     /* For entering first  ID */
        LPC_CANAF_RAM->mask[0] = 0x0000FFFF | (id << 16);
    }  else if (CAN_std_cnt == 1)  {             /* For entering second ID */
        if ((LPC_CANAF_RAM->mask[0] >> 16) > id)
            LPC_CANAF_RAM->mask[0] = (LPC_CANAF_RAM->mask[0] >> 16) | (id << 16);
        else
            LPC_CANAF_RAM->mask[0] = (LPC_CANAF_RAM->mask[0] & 0xFFFF0000) | id;
    }  else  {
        /* Find where to insert new ID */
        cnt1 = 0;
        cnt2 = CAN_std_cnt;
        bound1 = (CAN_std_cnt - 1) >> 1;
        while (cnt1 <= bound1)  {                  /* Loop through standard existing IDs */
            if ((LPC_CANAF_RAM->mask[cnt1] >> 16) > id)  {
                cnt2 = cnt1 * 2;
                break;
            }
            if ((LPC_CANAF_RAM->mask[cnt1] & 0x0000FFFF) > id)  {
                cnt2 = cnt1 * 2 + 1;
                break;
            }
            cnt1++;                                  /* cnt1 = U32 where to insert new ID */
        }                                          /* cnt2 = U16 where to insert new ID */
 
        if (cnt1 > bound1)  {                      /* Adding ID as last entry */
            if ((CAN_std_cnt & 0x0001) == 0)         /* Even number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1]  = 0x0000FFFF | (id << 16);
            else                                     /* Odd  number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1]  = (LPC_CANAF_RAM->mask[cnt1] & 0xFFFF0000) | id;
        }  else  {
            buf0 = LPC_CANAF_RAM->mask[cnt1];        /* Remember current entry */
            if ((cnt2 & 0x0001) == 0)                /* Insert new mask to even address */
                buf1 = (id << 16) | (buf0 >> 16);
            else                                     /* Insert new mask to odd  address */
                buf1 = (buf0 & 0xFFFF0000) | id;
 
            LPC_CANAF_RAM->mask[cnt1] = buf1;        /* Insert mask */
 
            bound1 = CAN_std_cnt >> 1;
            /* Move all remaining standard mask entries one place up */
            while (cnt1 < bound1)  {
                cnt1++;
                buf1  = LPC_CANAF_RAM->mask[cnt1];
                LPC_CANAF_RAM->mask[cnt1] = (buf1 >> 16) | (buf0 << 16);
                buf0  = buf1;
            }
 
            if ((CAN_std_cnt & 0x0001) == 0)         /* Even number of IDs exists */
                LPC_CANAF_RAM->mask[cnt1] = (LPC_CANAF_RAM->mask[cnt1] & 0xFFFF0000) | (0x0000FFFF);
        }
    }
    CAN_std_cnt++;
 
    /* Calculate std ID start address (buf0) and ext ID start address <- none (buf1) */
    buf0 = ((CAN_std_cnt + 1) >> 1) << 2;
    buf1 = buf0;
 
    /* Setup acceptance filter pointers */
    LPC_CANAF->SFF_sa     = 0;
    LPC_CANAF->SFF_GRP_sa = buf0;
    LPC_CANAF->EFF_sa     = buf0;
    LPC_CANAF->EFF_GRP_sa = buf1;
    LPC_CANAF->ENDofTable = buf1;
 
    LPC_CANAF->AFMR = 0x00000000;                  /* Use acceptance filter */
} // CAN2_wrFilter