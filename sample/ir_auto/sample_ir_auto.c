
#include <sys/time.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpi_isp.h"
#include "hi_comm_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_sys.h"

typedef struct
{
    HI_BOOL   bThreadFlag;
    pthread_t pThread;
} HI_IR_AUTO_THREAD_S;
static HI_IR_AUTO_THREAD_S g_astIrThread[ISP_MAX_DEV_NUM] = {{0}};

static ISP_IR_AUTO_ATTR_S g_astIrAttr[ISP_MAX_DEV_NUM] = {{0}};
static ISP_DEV IspDev = 0;

#define SAMPLE_IR_CALIBRATION_MODE  (0)
#define SAMPLE_IR_AUTO_MODE         (1)
#define GAIN_MAX_COEF               (280)
#define GAIN_MIN_COEF               (190)

static HI_S32 SAMPLE_IR_Set_Reg(HI_U32 u32Addr,HI_U32 u32Value)
{
    HI_U32 *pu32Addr = NULL;
    HI_U32 u32MapLen = sizeof(u32Value);

    pu32Addr = (HI_U32 *)HI_MPI_SYS_Mmap(u32Addr, u32MapLen);
    if(NULL == pu32Addr)
    {
        return HI_FAILURE;
    }

    *pu32Addr = u32Value;

    return HI_MPI_SYS_Munmap(pu32Addr, u32MapLen);
}

void SAMPLE_IR_AUTO_Usage(char *sPrgNm)
{
    printf("Usage : %s <mode> <Normal2IrExpThr> <Ir2NormalExpThr> <RGMax> <RGMin> <BGMax> <BGMin> <IrStatus>\n", sPrgNm);
    printf("mode:\n");
    printf("\t 0) SAMPLE_IR_CALIBRATION_MODE.\n");
    printf("\t 1) SAMPLE_IR_AUTO_MODE.\n");

    printf("Normal2IrExpThr:\n");
    printf("\t ISO threshold of switching from normal to IR mode.\n");

    printf("Ir2NormalExpThr:\n");
    printf("\t ISO threshold of switching from IR to normal mode.\n");

    printf("RGMax/RGMin/BGMax/BGMin:\n");
    printf("\t Maximum(Minimum) value of R/G(B/G) in IR scene.\n");

    printf("IrStatus:\n");
    printf("\t Current IR status. 0: Normal mode; 1: IR mode.\n");

    printf("e.g : %s 0 (SAMPLE_IR_CALIBRATION_MODE)\n", sPrgNm);
    printf("e.g : %s 1 (SAMPLE_IR_AUTO_MODE, default parameters)\n", sPrgNm);
    printf("e.g : %s 1 16000 400 280 190 280 190 0 (SAMPLE_IR_AUTO_MODE, user_define parameters)\n", sPrgNm);

    return;
}

HI_S32 ISP_IrSwitchToNormal(ISP_DEV IspDev)
{
    HI_S32 s32Ret;
    ISP_SATURATION_ATTR_S stIspSaturationAttr;
    ISP_WB_ATTR_S stIspWbAttr;
    ISP_COLORMATRIX_ATTR_S stIspCCMAttr;

    /* switch ir-cut to normal */
    SAMPLE_IR_Set_Reg(0x200f0100, 0x1);
    SAMPLE_IR_Set_Reg(0x200f0104, 0x1);
    SAMPLE_IR_Set_Reg(0x201C0400, 0x3);
    SAMPLE_IR_Set_Reg(0x201c000c, 0x2);
    usleep(1000000);
    SAMPLE_IR_Set_Reg(0x201c000c, 0x0);

    /* switch isp config to normal */
    s32Ret = HI_MPI_ISP_GetSaturationAttr(IspDev, &stIspSaturationAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetSaturationAttr failed\n");
        return s32Ret;
    }
    stIspSaturationAttr.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &stIspSaturationAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetSaturationAttr failed\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_ISP_GetWBAttr(IspDev, &stIspWbAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetWBAttr failed\n");
        return s32Ret;
    }
    stIspWbAttr.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetWBAttr(IspDev, &stIspWbAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetWBAttr failed\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_ISP_GetCCMAttr(IspDev, &stIspCCMAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetCCMAttr failed\n");
        return s32Ret;
    }
    stIspCCMAttr.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetCCMAttr(IspDev, &stIspCCMAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetCCMAttr failed\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_IrSwitchToIr(ISP_DEV IspDev)
{
    HI_S32 s32Ret;
    ISP_SATURATION_ATTR_S stIspSaturationAttr;
    ISP_WB_ATTR_S stIspWbAttr;
    ISP_COLORMATRIX_ATTR_S stIspCCMAttr;

    /* switch isp config to ir */
    s32Ret = HI_MPI_ISP_GetSaturationAttr(IspDev, &stIspSaturationAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetSaturationAttr failed\n");
        return s32Ret;
    }
    stIspSaturationAttr.enOpType = OP_TYPE_MANUAL;
    stIspSaturationAttr.stManual.u8Saturation = 0;
    s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &stIspSaturationAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetSaturationAttr failed\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_ISP_GetWBAttr(IspDev, &stIspWbAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetWBAttr failed\n");
        return s32Ret;
    }
    stIspWbAttr.enOpType = OP_TYPE_MANUAL;
    stIspWbAttr.stManual.u16Bgain  = 0x100;
    stIspWbAttr.stManual.u16Gbgain = 0x100;
    stIspWbAttr.stManual.u16Grgain = 0x100;
    stIspWbAttr.stManual.u16Rgain  = 0x100;
    s32Ret = HI_MPI_ISP_SetWBAttr(IspDev, &stIspWbAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetWBAttr failed\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_ISP_GetCCMAttr(IspDev, &stIspCCMAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetCCMAttr failed\n");
        return s32Ret;
    }
    stIspCCMAttr.enOpType = OP_TYPE_MANUAL;
    stIspCCMAttr.stManual.au16CCM[0] = 0x100;
    stIspCCMAttr.stManual.au16CCM[1] = 0x0;
    stIspCCMAttr.stManual.au16CCM[2] = 0x0;
    stIspCCMAttr.stManual.au16CCM[3] = 0x0;
    stIspCCMAttr.stManual.au16CCM[4] = 0x100;
    stIspCCMAttr.stManual.au16CCM[5] = 0x0;
    stIspCCMAttr.stManual.au16CCM[6] = 0x0;
    stIspCCMAttr.stManual.au16CCM[7] = 0x0;
    stIspCCMAttr.stManual.au16CCM[8] = 0x100;
    s32Ret = HI_MPI_ISP_SetCCMAttr(IspDev, &stIspCCMAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetCCMAttr failed\n");
        return s32Ret;
    }

    usleep(1000000);

    /* switch ir-cut to ir */
    SAMPLE_IR_Set_Reg(0x200f0100, 0x1);
    SAMPLE_IR_Set_Reg(0x200f0104, 0x1);
    SAMPLE_IR_Set_Reg(0x201C0400, 0x3);
    SAMPLE_IR_Set_Reg(0x201c000c, 0x1);
    usleep(1000000);
    SAMPLE_IR_Set_Reg(0x201c000c, 0x0);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_ISP_IrAutoRun(ISP_DEV IspDev)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_IR_AUTO_ATTR_S stIrAttr;

    stIrAttr.bEnable            = g_astIrAttr[IspDev].bEnable;
    stIrAttr.u32Normal2IrIsoThr = g_astIrAttr[IspDev].u32Normal2IrIsoThr;
    stIrAttr.u32Ir2NormalIsoThr = g_astIrAttr[IspDev].u32Ir2NormalIsoThr;
    stIrAttr.u32RGMax           = g_astIrAttr[IspDev].u32RGMax;
    stIrAttr.u32RGMin           = g_astIrAttr[IspDev].u32RGMin;
    stIrAttr.u32BGMax           = g_astIrAttr[IspDev].u32BGMax;
    stIrAttr.u32BGMin           = g_astIrAttr[IspDev].u32BGMin;
    stIrAttr.enIrStatus         = g_astIrAttr[IspDev].enIrStatus;

    while (HI_TRUE == g_astIrThread[IspDev].bThreadFlag)
    {
        /* run_interval: 40 ms */
        usleep(40000);

        s32Ret = HI_MPI_ISP_IrAutoRunOnce(IspDev, &stIrAttr);
        if (HI_SUCCESS != s32Ret)
        {
            printf("HI_MPI_ISP_IrAutoRunOnce failed\n");
            return s32Ret;
        }

        if (ISP_IR_SWITCH_TO_IR == stIrAttr.enIrSwitch) /* Normal to IR */
        {
            printf("\n[Normal -> IR]\n");

            s32Ret = ISP_IrSwitchToIr(IspDev);
            if (HI_SUCCESS != s32Ret)
            {
                printf("ISP_IrSwitchToIr failed\n");
                return s32Ret;
            }

            stIrAttr.enIrStatus = ISP_IR_STATUS_IR;
        }
        else if (ISP_IR_SWITCH_TO_NORMAL == stIrAttr.enIrSwitch) /* IR to Normal */
        {
            printf("\n[IR -> Normal]\n");

            s32Ret = ISP_IrSwitchToNormal(IspDev);
            if (HI_SUCCESS != s32Ret)
            {
                printf("ISP_IrSwitchToNormal failed\n");
                return s32Ret;
            }

            stIrAttr.enIrStatus = ISP_IR_STATUS_NORMAL;
        }
        else
        {}
    }

    return HI_SUCCESS;
}

static void *SAMPLE_ISP_IrAutoThread(void *param)
{
    ISP_DEV IspDev;

    IspDev = (ISP_DEV)param;

    prctl(PR_SET_NAME, (unsigned long)"isp_IrAuto", 0, 0, 0);

    SAMPLE_ISP_IrAutoRun(IspDev);

    return NULL;
}

HI_S32 SAMPLE_ISP_IrAutoExit(ISP_DEV IspDev)
{
    if (g_astIrThread[IspDev].pThread)
    {
        g_astIrThread[IspDev].bThreadFlag = HI_FALSE;
        pthread_join(g_astIrThread[IspDev].pThread, NULL);
        g_astIrThread[IspDev].pThread = 0;
    }

    return HI_SUCCESS;
}

void SAMPLE_IR_AUTO_HandleSig(HI_S32 signo)
{
    int ret;

    if (SIGINT == signo || SIGTERM == signo)
    {
        ret = SAMPLE_ISP_IrAutoExit(IspDev);
        if (ret != 0)
        {
            printf("SAMPLE_ISP_IrAutoExit failed\n");
            exit(-1);
        }
    }
    exit(-1);
}


#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32Mode = SAMPLE_IR_AUTO_MODE;
    IspDev = 0;

    /* need to modified when sensor/lens/IR-cut/Infrared light changed */
    g_astIrAttr[IspDev].bEnable = HI_TRUE;
    g_astIrAttr[IspDev].u32Normal2IrIsoThr = 16000;
    g_astIrAttr[IspDev].u32Ir2NormalIsoThr = 400;
    g_astIrAttr[IspDev].u32RGMax = 280;
    g_astIrAttr[IspDev].u32RGMin = 190;
    g_astIrAttr[IspDev].u32BGMax = 280;
    g_astIrAttr[IspDev].u32BGMin = 190;
    g_astIrAttr[IspDev].enIrStatus = ISP_IR_STATUS_NORMAL;

#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_IR_AUTO_HandleSig);
    signal(SIGTERM, SAMPLE_IR_AUTO_HandleSig);
#endif

    if (argc < 2)
    {
        SAMPLE_IR_AUTO_Usage(argv[0]);
        exit(HI_SUCCESS);
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_IR_AUTO_Usage(argv[0]);
        exit(HI_SUCCESS);
    }
    else
    {
        u32Mode = atoi(argv[1]);
    }

    if ((u32Mode != SAMPLE_IR_AUTO_MODE) && (u32Mode != SAMPLE_IR_CALIBRATION_MODE))
    {
        printf("the mode is invaild!\n");
        SAMPLE_IR_AUTO_Usage(argv[0]);
        exit(HI_SUCCESS);
    }

    /* SAMPLE_IR_CALIBRATION_MODE */
    if (SAMPLE_IR_CALIBRATION_MODE == u32Mode)
    {
        ISP_STATISTICS_S stStat;
        HI_U32 u32RG, u32BG;

        /* 1. Infrared scene without visible light */

        /* 2. Switch to IR */
        s32Ret = ISP_IrSwitchToIr(IspDev);
        if (HI_SUCCESS != s32Ret)
        {
            printf("ISP_IrSwitchToIr failed\n");
            return s32Ret;
        }

        /* 3. waiting for AE to stabilize */
        usleep(1000000);

        /* 4. calculate RGMax/RGMin/BGMax/BGMin */
        s32Ret = HI_MPI_ISP_GetStatistics(IspDev, &stStat);
        if (HI_SUCCESS != s32Ret)
        {
            printf("HI_MPI_ISP_GetStatistics failed\n");
            return s32Ret;
        }

        u32RG = ((HI_U32)stStat.stWBStat.stBayerStatistics.u16GlobalR << 8) / DIV_0_TO_1(stStat.stWBStat.stBayerStatistics.u16GlobalG);
        u32BG = ((HI_U32)stStat.stWBStat.stBayerStatistics.u16GlobalB << 8) / DIV_0_TO_1(stStat.stWBStat.stBayerStatistics.u16GlobalG);
        printf("RGMax:%d, RGMin:%d, BGMax:%d, BGMin:%d\n",
               u32RG * GAIN_MAX_COEF >> 8, u32RG * GAIN_MIN_COEF >> 8,
               u32BG * GAIN_MAX_COEF >> 8, u32BG * GAIN_MIN_COEF >> 8);
        return HI_SUCCESS;
    }

    /* SAMPLE_IR_AUTO_MODE */
    if (argc > 8)
    {
        g_astIrAttr[IspDev].u32Normal2IrIsoThr = atoi(argv[2]);
        g_astIrAttr[IspDev].u32Ir2NormalIsoThr = atoi(argv[3]);
        g_astIrAttr[IspDev].u32RGMax = atoi(argv[4]);
        g_astIrAttr[IspDev].u32RGMin = atoi(argv[5]);
        g_astIrAttr[IspDev].u32BGMax = atoi(argv[6]);
        g_astIrAttr[IspDev].u32BGMin = atoi(argv[7]);
        g_astIrAttr[IspDev].enIrStatus = atoi(argv[8]);
        if ((g_astIrAttr[IspDev].enIrStatus != ISP_IR_STATUS_NORMAL) && (g_astIrAttr[IspDev].enIrStatus != ISP_IR_STATUS_IR))
        {
            printf("the mode is invaild!\n");
            SAMPLE_IR_AUTO_Usage(argv[0]);
            exit(HI_SUCCESS);
        }

        printf("[UserConfig] Normal2IrIsoThr:%u, Ir2NormalIsoThr:%u, RG:[%u,%u], BG:[%u,%u], IrStatus:%d\n",
               g_astIrAttr[IspDev].u32Normal2IrIsoThr, g_astIrAttr[IspDev].u32Ir2NormalIsoThr,
               g_astIrAttr[IspDev].u32RGMin, g_astIrAttr[IspDev].u32RGMax,
               g_astIrAttr[IspDev].u32BGMin, g_astIrAttr[IspDev].u32BGMax,
               g_astIrAttr[IspDev].enIrStatus);
    }

    g_astIrThread[IspDev].bThreadFlag = HI_TRUE;

    s32Ret = pthread_create(&g_astIrThread[IspDev].pThread, HI_NULL, SAMPLE_ISP_IrAutoThread, (HI_VOID *)IspDev);
    if (0 != s32Ret)
    {
        printf("%s: create isp ir_auto thread failed!, error: %d, %s\r\n", __FUNCTION__, s32Ret, strerror(s32Ret));
        return HI_FAILURE;
    }

    printf("---------------press any key to exit!---------------\n");
    getchar();

    SAMPLE_ISP_IrAutoExit(IspDev);

    return HI_SUCCESS;
}


