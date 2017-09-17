/* codem16.c */
/*****************************************************************************/
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Mitsubishi M16                                              */
/*                                                                           */
/* Historie: 27.12.1996 Grundsteinlegung                                     */
/*            3. 1.1999 ChkPC-Anpassung                                      */
/*            9. 3.2000 'ambigious else'-Warnungen beseitigt                 */
/*                                                                           */
/*****************************************************************************/
/* $Id: codem16.c,v 1.34 2015/05/25 08:38:36 alfred Exp $                     */
/*****************************************************************************
 * $Log: codem16.c,v $
 * Revision 1.34  2015/05/25 08:38:36  alfred
 * - silence come warnings on old GCC versions
 *
 * Revision 1.33  2014/12/07 19:14:01  alfred
 * - silence a couple of Borland C related warnings and errors
 *
 * Revision 1.32  2014/12/05 11:58:16  alfred
 * - collapse STDC queries into one file
 *
 * Revision 1.31  2014/11/23 18:38:23  alfred
 * - use sizeof()
 *
 * Revision 1.30  2014/11/16 13:15:08  alfred
 * - remove some superfluous semicolons
 *
 * Revision 1.29  2014/07/02 20:55:42  alfred
 * - complete rework of M16
 *
 * Revision 1.28  2014/06/29 18:49:18  alfred
 * - rework ENTER/EXITD
 *
 * Revision 1.27  2014/06/29 17:36:43  alfred
 * - rework TRAP
 *
 * Revision 1.26  2014/06/29 14:40:21  alfred
 * - rework branches
 *
 * Revision 1.25  2014/06/29 12:55:06  alfred
 * - completed reworking bit operations
 *
 * Revision 1.24  2014/06/29 12:46:25  alfred
 * - rework EXT(U)
 *
 * Revision 1.23  2014/06/29 12:28:00  alfred
 * - rework bit field orders
 *
 * Revision 1.22  2014/06/29 10:08:06  alfred
 * - correct bit op formats
 * - rework bit op orders
 *
 * Revision 1.21  2014/06/28 20:42:23  alfred
 * - rework WAIT
 *
 * Revision 1.20  2014/06/25 17:21:06  alfred
 * - rework MULX/DIVX
 *
 * Revision 1.19  2014/06/23 17:35:35  alfred
 * - rework CSI
 *
 * Revision 1.18  2014/06/22 08:24:50  alfred
 * - rework CHK
 *
 * Revision 1.17  2014/06/21 19:45:47  alfred
 * - got through all arithmetic orders
 *
 * Revision 1.16  2014/06/21 14:36:57  alfred
 * - rework GE2Orders
 *
 * Revision 1.15  2014/06/21 13:21:32  alfred
 * - rework CMP, fix operand size check foer CMP:L
 *
 * Revision 1.14  2014/06/21 12:23:53  alfred
 * - avoid infinite loop in chained mode
 * - work around -0x8000 problem on Turbo C
 *
 * Revision 1.13  2014/06/20 18:04:21  alfred
 * - converted ADD/SUB
 *
 * Revision 1.12  2014/06/19 11:35:25  alfred
 * - one more reowrk on M16
 *
 * Revision 1.11  2014/06/16 20:05:20  alfred
 * - reowrk MOV instruction
 *
 * Revision 1.10  2014/06/15 12:17:30  alfred
 * - some more M16 reworks
 *
 * Revision 1.9  2014/06/15 09:18:32  alfred
 * - first instruction reworks on M16
 *
 * Revision 1.8  2014/06/14 17:28:43  alfred
 * - first steps to clean up M16 decoder
 *
 * Revision 1.7  2014/06/14 16:18:58  alfred
 * - correct array initialization
 *
 * Revision 1.6  2010/08/27 14:52:42  alfred
 * - some more overlapping strcpy() cleanups
 *
 * Revision 1.5  2007/11/24 22:48:07  alfred
 * - some NetBSD changes
 *
 * Revision 1.4  2005/10/02 10:00:45  alfred
 * - ConstLongInt gets default base, correct length check on KCPSM3 registers
 *
 * Revision 1.3  2005/09/08 17:06:29  alfred
 * - dynamically allocate string
 *
 * Revision 1.2  2004/05/29 11:33:03  alfred
 * - relocated DecodeIntelPseudo() into own module
 *
 *****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "nls.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "intconsts.h"

#define ModNone      (-1)
#define ModReg       0
#define MModReg      (1 << ModReg)
#define ModIReg      1
#define MModIReg     (1 << ModIReg)
#define ModDisp16    2
#define MModDisp16   (1 << ModDisp16)
#define ModDisp32    3
#define MModDisp32   (1 << ModDisp32)
#define ModImm       4
#define MModImm      (1 << ModImm)
#define ModAbs16     5
#define MModAbs16    (1 << ModAbs16)
#define ModAbs32     6
#define MModAbs32    (1 << ModAbs32)
#define ModPCRel16   7
#define MModPCRel16  (1 << ModPCRel16)
#define ModPCRel32   8
#define MModPCRel32  (1 << ModPCRel32)
#define ModPop       9
#define MModPop      (1 << ModPop)
#define ModPush      10
#define MModPush     (1 << ModPush)
#define ModRegChain  11
#define MModRegChain (1 << ModRegChain)
#define ModPCChain   12
#define MModPCChain  (1 << ModPCChain)
#define ModAbsChain  13
#define MModAbsChain (1 << ModAbsChain)

#define Mask_RegOnly    (MModReg)
#define Mask_AllShort   (MModReg | MModIReg | MModDisp16 | MModImm | MModAbs16 | MModAbs32 | MModPCRel16 | MModPCRel32 | MModPop | MModPush | MModPCChain | MModAbsChain)
#define Mask_AllGen     (Mask_AllShort | MModDisp32 | MModRegChain)
#define Mask_NoImmShort (Mask_AllShort & ~MModImm)
#define Mask_NoImmGen   (Mask_AllGen & ~MModImm)
#define Mask_MemShort   (Mask_NoImmShort & ~MModReg)
#define Mask_MemGen     (Mask_NoImmGen & ~MModReg)

#define Mask_Source     (Mask_AllGen & ~MModPush)
#define Mask_Dest       (Mask_NoImmGen & ~MModPop)
#define Mask_PureDest   (Mask_NoImmGen & ~(MModPush | MModPop))
#define Mask_PureMem    (Mask_MemGen & ~(MModPush | MModPop))

#define FixedLongOrderCount 2
#define OneOrderCount 13
#define GE2OrderCount 11
#define BitOrderCount 6
#define ConditionCount 14

typedef struct
{
  Word Mask;
  Byte OpMask;
  Word Code;
} OneOrder;

typedef struct
{
  Word Mask1,Mask2;
  Word SMask1,SMask2;
  Word Code;
  Boolean Signed;
} GE2Order;

typedef struct
{
  char *Name;
  Boolean MustByte;
  Word Code1,Code2;
} BitOrder;


static CPUVar CPUM16;

static char *Format;
static Byte FormatCode;
static ShortInt DOpSize,OpSize[5];
static Word AdrMode[5];
static ShortInt AdrType[5];
static Byte AdrCnt1[5],AdrCnt2[5];
static Word AdrVals[5][8];

static Byte OptionCnt;
static char Options[2][5];

static BitOrder *FixedLongOrders;
static OneOrder *OneOrders;
static GE2Order *GE2Orders;
static BitOrder *BitOrders;
static char **Conditions;

/*------------------------------------------------------------------------*/

typedef enum
{
  DispSizeNone,
  DispSize4,
  DispSize4Eps,
  DispSize16,
  DispSize32
} DispSize;

typedef struct _TChainRec
{
  struct _TChainRec *Next;
  Byte RegCnt;
  Word Regs[5],Scales[5];
  LongInt DispAcc;
  Boolean HasDisp;
  DispSize DSize;
} *PChainRec, TChainRec;
static Boolean ErrFlag;

static Boolean IsD4(LongInt inp)
{
  return ((inp >= -32) && (inp <= 28));
}

static Boolean IsD16(LongInt inp)
{
  return ((inp >= -32768) && (inp <= 32767));
}

static Boolean DecodeReg(char *Asc, Word *Erg)
{
  Boolean IO;

  if (!strcasecmp(Asc, "SP"))
    *Erg = 15;
  else if (!strcasecmp(Asc, "FP"))
    *Erg = 14;
  else if ((strlen(Asc) > 1) && (mytoupper(*Asc)=='R'))
  {
    *Erg = ConstLongInt(Asc + 1, &IO, 10);
    return ((IO) && (*Erg <= 15));
  }
  else
    return False;
  return True;
}

static void SplitSize(char *s, DispSize *Erg)
{
  int l = strlen(s);

  if ((l > 2) && (!strcmp(s + (l - 2), ":4")))
  {
    *Erg = DispSize4;
    s[l - 2] = '\0';
  }
  else if ((l > 3) && (!strcmp(s + (l - 3), ":16")))
  {
    *Erg = DispSize16;
    s[l - 3] = '\0';
  }
  else if ((l > 3) && (!strcmp(s + (l - 3), ":32")))
  {
    *Erg = DispSize32;
    s[l - 3] = '\0';
  }
}

static void DecideAbs(LongInt Disp, DispSize Size, Word Mask, int Index)
{
  switch (Size)
  {
    case DispSize4:
      Size = DispSize16;
      break;
    case DispSizeNone:
      Size = ((IsD16(Disp)) && (Mask & MModAbs16)) ? DispSize16 : DispSize32;
      break;
    default:
      break;
  }

  switch (Size)
  {
    case DispSize16:
      if (ChkRange(Disp, -32768, 32767))
      {
        AdrType[Index] = ModAbs16;
        AdrMode[Index] = 0x09;
        AdrVals[Index][0] = Disp & 0xffff;
        AdrCnt1[Index] = 2;
      }
      break;
    case DispSize32:
      AdrType[Index] = ModAbs32;
      AdrMode[Index] = 0x0a;
      AdrVals[Index][0] = Disp >> 16;
      AdrVals[Index][1] = Disp & 0xffff;
      AdrCnt1[Index] = 4;
      break;
    default:
      WrError(10000);
  }
}

static void SetError(Word Code)
{
  WrError(Code);
  ErrFlag = True;
}

static PChainRec DecodeChain(char *Asc)
{
  PChainRec Rec;
  String Part,SReg;
  int z;
  char *p;
  Boolean OK;
  Byte Scale;

  ChkStack();
  Rec = (PChainRec) malloc(sizeof(TChainRec));
  Rec->Next = NULL;
  Rec->RegCnt = 0;
  Rec->DispAcc = 0;
  Rec->HasDisp = False;
  Rec->DSize = DispSizeNone;

  while ((*Asc != '\0') && (!ErrFlag))
  {

    /* eine Komponente abspalten */

    p=QuotPos(Asc, ',');
    if (!p)
    {
      strmaxcpy(Part, Asc, 255);
      *Asc = '\0';
    }
    else
    {
      *p = '\0';
      strmaxcpy(Part, Asc, 255);
      strmov(Asc, p + 1);
    }

    strcpy(SReg, Part);
    p = QuotPos(SReg, '*');
    if (p)
      *p = '\0';

    /* weitere Indirektion ? */

    if (*Part == '@')
    {
      if (Rec->Next) SetError(1350);
      else
      {
        strmov(Part, Part + 1);
        if (IsIndirect(Part))
        {
          strmov(Part, Part + 1);
          Part[strlen(Part) - 1] = '\0';
        }
        Rec->Next = DecodeChain(Part);
      }
    }

    /* Register, mit Skalierungsfaktor ? */

    else if (DecodeReg(SReg, Rec->Regs + Rec->RegCnt))
    {
      if (Rec->RegCnt >= 5) SetError(1350);
      else
      {
        FirstPassUnknown = False;
        if (!p)
        {
          OK = True;
          Scale = 1;
        }
        else
          Scale = EvalIntExpression(p + 1, UInt4, &OK);
        if (FirstPassUnknown)
          Scale = 1;
        if (!OK) ErrFlag = True;
        else if ((Scale != 1) && (Scale != 2) && (Scale != 4) && (Scale != 8)) SetError(1350);
        else
        {
          Rec->Scales[Rec->RegCnt] = 0;
          while (Scale > 1)
          {
            Rec->Scales[Rec->RegCnt]++;
            Scale = Scale >> 1;
          }
          Rec->RegCnt++;
        }
      }
    }

    /* PC, mit Skalierungsfaktor ? */

    else if (!strcasecmp(SReg, "PC"))
    {
      if (Rec->RegCnt >= 5) SetError(1350);
      else
      {
        FirstPassUnknown = False;
        if (!p)
        {
          OK = True;
          Scale = 1;
        }
        else
          Scale = EvalIntExpression(p + 1, UInt4, &OK);
        if (FirstPassUnknown)
          Scale = 1;
        if (!OK) ErrFlag = True;
        else if ((Scale != 1) && (Scale != 2) && (Scale != 4) && (Scale != 8)) SetError(1350);
        else
        {
          for (z = Rec->RegCnt - 1; z >= 0; z--)
          {
            Rec->Regs[z + 1] = Rec->Regs[z];
            Rec->Scales[z + 1] = Rec->Scales[z];
          }
          Rec->Scales[0] = 0;
          while (Scale > 1)
          {
            Rec->Scales[0]++;
            Scale = Scale >> 1;
          }
          Rec->Regs[0] = 16;
          Rec->RegCnt++;
        }
      }
    }

    /* ansonsten Displacement */

    else
    {
      SplitSize(Part, &(Rec->DSize));
      Rec->DispAcc += EvalIntExpression(Part, Int32, &OK);
      if (!OK) ErrFlag = True;
      Rec->HasDisp = True;
    }
  }

  if (ErrFlag)
  {
    free(Rec);
    return NULL;
  }
  else return Rec;
}

static Boolean ChkAdr(Word Mask, int Index)
{
  AdrCnt2[Index]=AdrCnt1[Index] >> 1;
  if ((AdrType[Index]!=-1) && ((Mask & (1 << AdrType[Index]))==0))
  {
    char Str[30];

    AdrCnt1[Index] = AdrCnt2[Index] = 0;
    AdrType[Index] = ModNone;
    sprintf(Str, "%d", Index);
    WrXError(1350, Str);
    return False;
  }
  else
    return (AdrType[Index] != ModNone);
}

static Boolean DecodeAdr(char *Asc, int Index, Word Mask)
{
  LongInt AdrLong, MinReserve, MaxReserve;
  int z, z2, LastChain;
  Boolean OK, Error;
  PChainRec RootChain, RunChain, PrevChain;
  DispSize DSize;

  AdrCnt1[Index] = 0;
  AdrType[Index] = ModNone;

  /* Register ? */

  if (DecodeReg(Asc, AdrMode + Index))
  {
    AdrType[Index] = ModReg;
    AdrMode[Index] += 0x10;
    return ChkAdr(Mask, Index);
  }

  /* immediate ? */

  if (*Asc == '#')
  {
    switch (OpSize[Index])
    {
      case -1:
        WrError(1132);
        OK = False;
        break;
      case 0:
        AdrVals[Index][0] = EvalIntExpression(Asc + 1, Int8, &OK) & 0xff;
        if (OK)
          AdrCnt1[Index] = 2;
        break;
      case 1:
        AdrVals[Index][0] = EvalIntExpression(Asc + 1, Int16, &OK);
        if (OK)
          AdrCnt1[Index] = 2;
        break;
      case 2:
        AdrLong = EvalIntExpression(Asc + 1, Int32, &OK);
        if (OK)
        {
          AdrVals[Index][0] = AdrLong >> 16;
          AdrVals[Index][1] = AdrLong & 0xffff;
          AdrCnt1[Index] = 4;
        }
        break;
    }
    if (OK)
    {
      AdrType[Index] = ModImm;
      AdrMode[Index] = 0x0c;
    }
    return ChkAdr(Mask, Index);
  }

  /* indirekt ? */

  if (*Asc == '@')
  {
    strmov(Asc, Asc + 1);
    if (IsIndirect(Asc))
    {
      strmov(Asc, Asc + 1);
      Asc[strlen(Asc) - 1] = '\0';
    }

    /* Stack Push ? */

    if ((!strcasecmp(Asc, "-R15")) || (!strcasecmp(Asc, "-SP")))
    {
      AdrType[Index] = ModPush;
      AdrMode[Index] = 0x05;
      return ChkAdr(Mask, Index);
    }

    /* Stack Pop ? */

    if ((!strcasecmp(Asc, "R15+")) || (!strcasecmp(Asc, "SP+")))
    {
      AdrType[Index] = ModPop;
      AdrMode[Index] = 0x04;
      return ChkAdr(Mask, Index);
    }

    /* Register einfach indirekt ? */

    if (DecodeReg(Asc, AdrMode + Index))
    {
      AdrType[Index] = ModIReg;
      AdrMode[Index] += 0x30;
      return ChkAdr(Mask, Index);
    }

    /* zusammengesetzt indirekt ? */

    ErrFlag = False;
    RootChain = DecodeChain(Asc);

    if (ErrFlag);

    else if (!RootChain);

    /* absolut ? */

    else if ((!RootChain->Next) && (RootChain->RegCnt == 0))
    {
      if (!RootChain->HasDisp)
        RootChain->DispAcc = 0;
      DecideAbs(RootChain->DispAcc, RootChain->DSize, Mask, Index);
      free(RootChain);
    }

    /* einfaches Register/PC mit Displacement ? */

    else if ((!RootChain->Next) && (RootChain->RegCnt == 1) && (RootChain->Scales[0] == 0))
    {
      if (RootChain->Regs[0] == 16)
        RootChain->DispAcc -= EProgCounter();

      /* Displacement-Groesse entscheiden */

      if (RootChain->DSize == DispSizeNone)
      {
        if ((RootChain->DispAcc == 0) && (RootChain->Regs[0] < 16));
        else
         RootChain->DSize = (IsD16(RootChain->DispAcc)) ? DispSize16 : DispSize32;
      }

      switch (RootChain->DSize)
      {
        /* kein Displacement mit Register */

        case DispSizeNone:
          if (ChkRange(RootChain->DispAcc, 0, 0))
          {
            if (RootChain->Regs[0] >= 16) WrError(1350);
            else
            {
              AdrType[Index] = ModIReg;
              AdrMode[Index] = 0x30 + RootChain->Regs[0];
            }
          }
          break;

        /* 16-Bit-Displacement */

        case DispSize4:
        case DispSize16:
          if (ChkRange(RootChain->DispAcc, -32768, 32767))
          {
            AdrVals[Index][0] = RootChain->DispAcc & 0xffff;
            AdrCnt1[Index] = 2;
            if (RootChain->Regs[0] == 16)
            {
              AdrType[Index] = ModPCRel16;
              AdrMode[Index] = 0x0d;
            }
            else
            {
              AdrType[Index] = ModDisp16;
              AdrMode[Index] = 0x20 + RootChain->Regs[0];
            }
          }
          break;

        /* 32-Bit-Displacement */

        default:
         AdrVals[Index][1] = RootChain->DispAcc & 0xffff;
         AdrVals[Index][0] = RootChain->DispAcc >> 16;
         AdrCnt1[Index] = 4;
         if (RootChain->Regs[0] == 16)
         {
           AdrType[Index] = ModPCRel32;
           AdrMode[Index] = 0x0e;
         }
         else
         {
           AdrType[Index] = ModDisp32;
           AdrMode[Index] = 0x40 + RootChain->Regs[0];
         }
      }

      free(RootChain);
    }

    /* komplex: dann chained iterieren */

    else
    {
      /* bis zum innersten Element der Indirektion als Basis laufen */

      RunChain=RootChain;
      while (RunChain->Next)
        RunChain = RunChain->Next;

      /* Entscheidung des Basismodus: die Basis darf nicht skaliert
         sein, und wenn ein Modus nicht erlaubt ist, muessen wir mit
         Base-none anfangen... */

      z = 0;
      while ((z < RunChain->RegCnt) && (RunChain->Scales[z] != 0))
        z++;
      if (z >= RunChain->RegCnt)
      {
        AdrType[Index] = ModAbsChain;
        AdrMode[Index] = 0x0b;
      }
      else
      {
        if (RunChain->Regs[z] == 16)
        {
          AdrType[Index] = ModPCChain;
          AdrMode[Index] = 0x0f;
          RunChain->DispAcc -= EProgCounter();
        }
        else
        {
          AdrType[Index] = ModRegChain;
          AdrMode[Index] = 0x60 + RunChain->Regs[z];
        }
        for (z2 = z; z2 <= RunChain->RegCnt - 2; z2++)
        {
          RunChain->Regs[z2] = RunChain->Regs[z2 + 1];
          RunChain->Scales[z2] = RunChain->Scales[z2 + 1];
        }
        RunChain->RegCnt--;
      }

      /* Jetzt ueber die einzelnen Komponenten iterieren */

      LastChain = 0;
      Error = False;
      while ((RootChain) && (!Error))
      {
        RunChain = RootChain;
        PrevChain = NULL;
        while (RunChain->Next)
        {
          PrevChain = RunChain;
          RunChain = RunChain->Next;
        }

        /* noch etwas abzulegen ? */

        if ((RunChain->RegCnt != 0) || (RunChain->HasDisp))
        {
          LastChain = AdrCnt1[Index] >> 1;

          /* Register ablegen */

          if (RunChain->RegCnt != 0)
          {
            AdrVals[Index][LastChain] =
              (RunChain->Regs[0] == 16) ? 0x0600 : RunChain->Regs[0] << 10;
            AdrVals[Index][LastChain] += RunChain->Scales[0] << 5;
            for (z2 = 0; z2 <= RunChain->RegCnt - 2; z2++)
            {
              RunChain->Regs[z2] = RunChain->Regs[z2 + 1];
              RunChain->Scales[z2] = RunChain->Scales[z2 + 1];
            }
            RunChain->RegCnt--;
          }
          else
            AdrVals[Index][LastChain] = 0x0200;
          AdrCnt1[Index] += 2;

          /* Displacement ablegen */

          if (RunChain->HasDisp)
          {
            if ((AdrVals[Index][LastChain] & 0x3e00) == 0x0600)
              RunChain->DispAcc -= EProgCounter();

            if (RunChain->DSize == DispSizeNone)
            {
              MinReserve = 32 * RunChain->RegCnt;
              MaxReserve = 28 * RunChain->RegCnt;
              if (IsD4(RunChain->DispAcc))
                DSize = ((RunChain->DispAcc & 3) == 0) ? DispSize4 : DispSize16;
              else if ((RunChain->DispAcc >= -32 - MinReserve)
                    && (RunChain->DispAcc <= 28 + MaxReserve))
                DSize = DispSize4Eps;
              else if (IsD16(RunChain->DispAcc))
                DSize = DispSize16;
              else if ((RunChain->DispAcc >= -32768 - MinReserve)
                    && (RunChain->DispAcc <= 32767 + MaxReserve))
                DSize = DispSize4Eps;
              else
                DSize = DispSize32;
            }
            else
              DSize = RunChain->DSize;
            RunChain->DSize = DispSizeNone;

            switch (DSize)
            {
              /* Fall 1: passt komplett in 4er-Displacement */

              case DispSize4:
                if (ChkRange(RunChain->DispAcc, -32, 28))
                {
                  if (RunChain->DispAcc & 3)
                  {
                    WrError(1325);
                    Error = True;
                  }
                  else
                  {
                    AdrVals[Index][LastChain] += (RunChain->DispAcc >> 2) & 0x000f;
                    RunChain->HasDisp = False;
                  }
                }
                break;

              /* Fall 2: passt nicht mehr in naechstkleineres Displacement, aber wir
                 koennen hier schon einen Teil ablegen, um im naechsten Iterations-
                 schritt ein kuerzeres Displacement zu bekommen */

              case DispSize4Eps:
                if (RunChain->DispAcc > 0)
                {
                  AdrVals[Index][LastChain] += 0x0007;
                  RunChain->DispAcc -= 28;
                }
                else
                {
                  AdrVals[Index][LastChain] += 0x0008;
                  RunChain->DispAcc += 32;
                }
                break;

              /* Fall 3: 16 Bit */

              case DispSize16:
                if (ChkRange(RunChain->DispAcc, -32768, 32767))
                {
                  AdrVals[Index][LastChain] += 0x0011;
                  AdrVals[Index][LastChain + 1] = RunChain->DispAcc & 0xffff;
                  AdrCnt1[Index] += 2;
                  RunChain->HasDisp = False;
                }
                else
                  Error = True;
                break;

              /* Fall 4: 32 Bit */

              case DispSize32:
                AdrVals[Index][LastChain] += 0x0012;
                AdrVals[Index][LastChain+1] = RunChain->DispAcc >> 16;
                AdrVals[Index][LastChain+2] = RunChain->DispAcc & 0xffff;
                AdrCnt1[Index] += 4;
                RunChain->HasDisp = False;
                break;

              default:
                WrError(10000);
                Error = True;
            }
          }
        }

        /* nichts mehr drin: dann ein leeres Steuerwort erzeugen.  Tritt
           auf, falls alles schon im Basisadressierungsbyte verschwunden */

        else if (RunChain != RootChain)
        {
          LastChain = AdrCnt1[Index] >> 1;
          AdrVals[Index][LastChain] = 0x0200;
          AdrCnt1[Index] += 2;
        }

        /* nichts mehr drin: wegwerfen
           wenn wir noch nicht ganz vorne angekommen sind, dann ein
           Indirektionsflag setzen */

        if ((RunChain->RegCnt == 0) && (!RunChain->HasDisp))
        {
          if (RunChain != RootChain)
            AdrVals[Index][LastChain] += 0x4000;
          if (!PrevChain)
            RootChain = NULL;
          else
            PrevChain->Next=NULL;
          free(RunChain);
        }
      }

      /* Ende-Kennung fuer letztes Glied */

      AdrVals[Index][LastChain] += 0x8000;
    }

    return ChkAdr(Mask, Index);
  }

  /* ansonsten absolut */

  DSize = DispSizeNone;
  SplitSize(Asc, &DSize);
  AdrLong=EvalIntExpression(Asc, Int32, &OK);
  if (OK)
    DecideAbs(AdrLong, DSize, Mask, Index);

  return ChkAdr(Mask, Index);
}

static LongInt ImmVal(int Index)
{
  switch (OpSize[Index])
  {
    case 0:
      return (ShortInt) (AdrVals[Index][0] & 0xff);
    case 1:
      return (Integer) (AdrVals[Index][0]);
    case 2:
      return (((LongInt)AdrVals[Index][0]) << 16)
            + ((Integer)AdrVals[Index][1]);
    default:
      WrError(10000);
      return 0;
  }
}

static Boolean IsShort(int Index)
{
  return (!(AdrMode[Index] & 0xc0));
}

static void AdaptImm(int Index, Byte NSize, Boolean Signed)
{
  switch (OpSize[Index])
  {
    case 0:
      if (NSize!=0)
      {
        if (((AdrVals[Index][0] & 0x80) == 0x80) && (Signed))
          AdrVals[Index][0] |= 0xff00;
        else
          AdrVals[Index][0] &= 0xff;
        if (NSize == 2)
        {
          if (((AdrVals[Index][0] & 0x8000) == 0x8000) && (Signed))
            AdrVals[Index][1] = 0xffff;
          else
            AdrVals[Index][1] = 0;
          AdrCnt1[Index] += 2;
          AdrCnt2[Index]++;
        }
      }
      break;
    case 1:
      if (NSize == 0)
        AdrVals[Index][0] &= 0xff;
      else if (NSize == 2)
      {
        if (((AdrVals[Index][0] & 0x8000) == 0x8000) && (Signed))
          AdrVals[Index][1] = 0xffff;
        else AdrVals[Index][1] = 0;
        AdrCnt1[Index] += 2;
        AdrCnt2[Index]++;
      }
      break;
    case 2:
      if (NSize != 2)
      {
        AdrCnt1[Index] -= 2;
        AdrCnt2[Index]--;
        if (NSize == 0)
          AdrVals[Index][0] &= 0xff;
      }
      break;
  }
  OpSize[Index] = NSize;
}

static ShortInt DefSize(Byte Mask)
{
  ShortInt z;

  z = 2;
  while ((z >= 0) && (!(Mask & 4)))
  {
    Mask = (Mask << 1) & 7;
    z--;
  }
  return z;
}

static Word RMask(Word No, Boolean Turn)
{
  return (Turn) ? (0x8000 >> No) : (1 << No);
}

static Boolean DecodeRegList(char *Asc, Word *Erg, Boolean Turn)
{
  char Part[11];
  char *p,*p1,*p2;
  Word r1,r2,z;

  if (IsIndirect(Asc))
  {
    strmov(Asc, Asc + 1); Asc[strlen(Asc) - 1] = '\0';
  }
  *Erg = 0;
  while (*Asc != '\0')
  {
    p1 = strchr(Asc,',');
    p2 = strchr(Asc,'/');
    p = ((p1) && (p1<p2)) ? p1 : p2;
    if (!p)
    {
      strmaxcpy(Part, Asc, sizeof(Part));
      *Asc = '\0';
    }
    else
    {
      *p = '\0';
      strmaxcpy(Part, Asc, sizeof(Part));
      strmov(Asc, p + 1);
    }
    p = strchr(Part, '-');
    if (!p)
    {
      if (!DecodeReg(Part, &r1))
      {
        WrXError(1410, Part);
        return False;
      }
      *Erg |= RMask(r1, Turn);
    }
    else
    {
      *p = '\0';
      if (!DecodeReg(Part, &r1))
      {
        WrXError(1410, Part);
        return False;
      }
      if (!DecodeReg(p + 1, &r2))
      {
        WrXError(1410, p + 1);
        return False;
      }
      if (r1 <= r2)
      {
        for (z = r1; z <= r2; z++)
          *Erg |= RMask(z, Turn);
      }
      else
      {
        for (z = r2; z <= 15; z++)
          *Erg |= RMask(z, Turn);
        for (z = 0; z <= r1; z++)
          *Erg |= RMask(z,Turn);
      }
    }
  }
  return True;
}

static Boolean DecodeCondition(const char *Asc, Word *Erg)
{
  int z;

  for (z = 0; z < ConditionCount; z++)
    if (!strcasecmp(Asc, Conditions[z]))
      break;
  *Erg = z;
  return (z < ConditionCount);
}

static Boolean DecodeStringCondition(const char *Asc, Word *pErg)
{
  Boolean OK = TRUE;

  if (!strcasecmp(Asc, "LTU"))
    *pErg = 0;
  else if (!strcasecmp(Asc, "GEU"))
    *pErg = 1;
  else if (!strcasecmp(Asc, "N"))
    *pErg = 6;
  else
    OK = (DecodeCondition(Asc, pErg) && (*pErg > 1) && (*pErg < 6));

  return OK;
}

/*------------------------------------------------------------------------*/

static Boolean CheckFormat(char *FSet)
{
  char *p;

  if (!strcmp(Format, " "))
    FormatCode = 0;
  else
  {
    p = strchr(FSet, *Format);
    if (p)
      FormatCode = p - FSet + 1;
    else
      WrError(1090);
    return (p != NULL);
  }
  return True;
}

static Boolean CheckBFieldFormat(void)
{
  if ((!strcmp(Format, "G:R")) || (!strcmp(Format, "R:G")))
    FormatCode = 1;
  else if ((!strcmp(Format, "G:I")) || (!strcmp(Format, "I:G")))
    FormatCode = 2;
  else if ((!strcmp(Format, "E:R")) || (!strcmp(Format, "R:E")))
    FormatCode = 3;
  else if ((!strcmp(Format, "E:I")) || (!strcmp(Format, "I:E")))
    FormatCode = 4;
  else
  {
    WrError(1090);
    return False;
  }
  return True;
}

static Boolean GetOpSize(char *Asc, Byte Index)
{
  char *p;
  int l = strlen(Asc);

  p = RQuotPos(Asc, '.');
  if (!p)
  {
    OpSize[Index] = DOpSize;
    return True;
  }
  else if (p==Asc+l-2)
  {
    switch (mytoupper(p[1]))
    {
      case 'B':
        OpSize[Index] = 0;
        break;
      case 'H':
        OpSize[Index] = 1;
        break;
      case 'W':
        OpSize[Index] = 2;
        break;
      default:
        WrXError(1107, p);
        return False;
    }
    *p = '\0';
    return True;
  }
  else
  {
    WrError(1107);
    return False;
  }
}

static void SplitOptions(void)
{
  char *pSplit, *pSrc;

  pSrc = OpPart;
  *Options[0] = *Options[1] = '\0';
  for (OptionCnt = 0; OptionCnt < 2; OptionCnt++)
  {
    pSplit = QuotPos(pSrc, '/');
    if (!pSplit)
      break;
    strmaxcpy(Options[OptionCnt], pSplit + 1, 255);
    if (OptionCnt)
      *pSplit = '\0';
    else
      pSplit[1] = '\0';
    pSrc = Options[OptionCnt];
  }
}

static void DecideBranch(LongInt Adr, Byte Index)
{
  LongInt Dist = Adr - EProgCounter();

  if (FormatCode == 0)
  {
    /* Groessenangabe erzwingt G-Format */
    if (OpSize[Index] != -1)
      FormatCode = 1;
    /* gerade 9-Bit-Zahl kurz darstellbar */
    else if (((Dist & 1) == 0) && (Dist <= 254) && (Dist >= -256))
      FormatCode = 2;
    /* ansonsten allgemein */
    else
      FormatCode = 1;
  }
  if ((FormatCode == 1) && (OpSize[Index] == -1))
  {
    if ((Dist <= 127) && (Dist >= -128))
      OpSize[Index] = 0;
    else if ((Dist <= 32767) && (Dist >= -32768))
      OpSize[Index] = 1;
    else
      OpSize[Index] = 2;
  }
}

static Boolean DecideBranchLength(LongInt *Addr, int Index)
{
  *Addr -= EProgCounter();
  if (OpSize[Index] == -1)
  {
    if ((*Addr >= -128) && (*Addr <= 127))
      OpSize[Index] = 0;
    else if ((*Addr >= -32768) && (*Addr <= 32767))
      OpSize[Index] = 1;
    else OpSize[Index]=2;
  }

  if ((!SymbolQuestionable) &&
      (((OpSize[Index] == 0) && ((*Addr < -128) || (*Addr > 127)))
    || ((OpSize[Index] == 1) && ((*Addr < -32768) || (*Addr > 32767)))))
  {
    WrError(1370);
    return False;
  }
  else
    return True;
}

static void Make_G(Word Code)
{
  WAsmCode[0] = 0xd000 + (OpSize[1] << 8) + AdrMode[1];
  memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
  WAsmCode[1 + AdrCnt2[1]] = Code + (OpSize[2] << 8) + AdrMode[2];
  memcpy(WAsmCode + 2 + AdrCnt2[1], AdrVals[2], AdrCnt1[2]);
  CodeLen = 4 + AdrCnt1[1] + AdrCnt1[2];
}

static void Make_E(Word Code, Boolean Signed)
{
  LongInt HVal, Min, Max;

  Min = 128 * (-Ord(Signed));
  Max = Min + 255;
  if (AdrType[1] != ModImm) WrError(1350);
  else
  {
    HVal = ImmVal(1);
    if (ChkRange(HVal, Min, Max))
    {
      WAsmCode[0] = 0xbf00 + (HVal & 0xff);
      WAsmCode[1] = Code + (OpSize[2] << 8) + AdrMode[2];
      memcpy(WAsmCode + 2, AdrVals[2], AdrCnt1[2]);
      CodeLen = 4 + AdrCnt1[2];
    }
  }
}

static void Make_I(Word Code, Boolean Signed)
{
  if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
  else
  {
    AdaptImm(1, OpSize[2], Signed);
    WAsmCode[0] = Code + (OpSize[2] << 8) + AdrMode[2];
    memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
    memcpy(WAsmCode + 1 + AdrCnt2[2], AdrVals[1], AdrCnt1[1]);
    CodeLen = 2 + AdrCnt1[1] + AdrCnt1[2];
  }
}

/*------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ArgCnt != 0) WrError(1110);
  else if (*AttrPart != '\0') WrError(1100);
  else if (strcmp(Format, " ")) WrError(1090);
  else
  {
    CodeLen = 2;
    WAsmCode[0] = Code;
  }
}

static void DecodeACB_SCB(Word IsSCB)
{
  if (ArgCnt != 4) WrError(1110);
  else if (CheckFormat("GEQR"))
  {
    if (GetOpSize(ArgStr[2], 3))
    if (GetOpSize(ArgStr[4], 4))
    if (GetOpSize(ArgStr[1], 1))
    if (GetOpSize(ArgStr[3], 2))
    {
      Word HReg;

      if ((OpSize[3] == -1) && (OpSize[2] == -1)) OpSize[3] = 2;
      if ((OpSize[3] == -1) && (OpSize[2] != -1)) OpSize[3] = OpSize[2];
      else if ((OpSize[3] != -1) && (OpSize[2] == -1)) OpSize[2] = OpSize[3];
      if (OpSize[1] == -1) OpSize[1] = OpSize[2];
      if (OpSize[3] != OpSize[2]) WrError(1131);
      else if (!DecodeReg(ArgStr[2], &HReg)) WrError(1350);
      else
      {
        Boolean OK;
        LongInt AdrLong, HVal;

        AdrLong = EvalIntExpression(ArgStr[4], Int32, &OK);
        if (OK)
        {
          if (DecodeAdr(ArgStr[1], 1, Mask_Source))
          {
            if (DecodeAdr(ArgStr[3], 2, Mask_Source))
            {
              if (FormatCode == 0)
              {
                if (AdrType[1] != ModImm) FormatCode = 1;
                else
                {
                  HVal = ImmVal(1);
                  if ((HVal == 1) && (AdrType[2] == ModReg))
                    FormatCode = 4;
                  else if ((HVal == 1) && (AdrType[2] == ModImm))
                  {
                    HVal = ImmVal(2);
                    if ((HVal >= 1 - IsSCB) && (HVal <= 64 - IsSCB))
                      FormatCode = 3;
                    else
                      FormatCode = 2;
                  }
                  else if ((HVal >= -128) && (HVal <= 127))
                    FormatCode = 2;
                  else
                    FormatCode = 1;
                }
              }
              switch (FormatCode)
              {
                case 1:
                  if (DecideBranchLength(&AdrLong, 4))  /* ??? */
                  {
                    WAsmCode[0] = 0xd000 + (OpSize[1] << 8) + AdrMode[1];
                    memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
                    WAsmCode[1 + AdrCnt2[1]] = 0xf000 + (IsSCB << 11) + (OpSize[2] << 8) + AdrMode[2];
                    memcpy(WAsmCode + 2 + AdrCnt2[1], AdrVals[2], AdrCnt1[2]);
                    WAsmCode[2 + AdrCnt2[1] + AdrCnt2[2]] = (HReg << 10) + (OpSize[4] << 8);
                    CodeLen = 6 + AdrCnt1[1] + AdrCnt1[2];
                  }
                  break;
                case 2:
                  if (DecideBranchLength(&AdrLong, 4))  /* ??? */
                  {
                    if (AdrType[1] != ModImm) WrError(1350);
                    else
                    {
                      HVal = ImmVal(1);
                      if (ChkRange(HVal, -128, 127))
                      {
                        WAsmCode[0] = 0xbf00 + (HVal & 0xff);
                        WAsmCode[1] = 0xf000 + (IsSCB << 11) + (OpSize[2] << 8) + AdrMode[2];
                        memcpy(WAsmCode + 2, AdrVals[2], AdrCnt1[2]);
                        WAsmCode[2 + AdrCnt2[2]] = (HReg << 10) + (OpSize[4] << 8);
                        CodeLen = 6 + AdrCnt1[2];
                      }
                    }
                  }
                  break;
                case 3:
                  if (DecideBranchLength(&AdrLong, 4))  /* ??? */
                  {
                    if (AdrType[1] != ModImm) WrError(1350);
                    else if (ImmVal(1) != 1) WrError(1135);
                    else if (AdrType[2] != ModImm) WrError(1350);
                    else
                    {
                      HVal = ImmVal(2);
                      if (ChkRange(HVal, 1 - IsSCB, 64 - IsSCB))
                      {
                        WAsmCode[0] = 0x03d1 + (HReg << 10) + (IsSCB << 1);
                        WAsmCode[1] = ((HVal & 0x3f) << 10) + (OpSize[4] << 8);
                        CodeLen = 4;
                      }
                    }
                  }
                  break;
                case 4:
                  if (DecideBranchLength(&AdrLong, 4))  /* ??? */
                  {
                    if (AdrType[1] != ModImm) WrError(1350);
                    else if (ImmVal(1) != 1) WrError(1135);
                    else if (OpSize[2] != 2) WrError(1130);
                    else if (AdrType[2] != ModReg) WrError(1350);
                    else
                    {
                      WAsmCode[0] = 0x03d0 + (HReg << 10) + (IsSCB << 1);
                      WAsmCode[1] = ((AdrMode[2] & 15) << 10) + (OpSize[4] << 8);
                      CodeLen = 4;
                    }
                  }
                  break;
              }
              if (CodeLen>0)
              {
                switch (OpSize[4])
                {
                  case 0:
                    WAsmCode[(CodeLen >> 1) - 1] += AdrLong & 0xff;
                    break;
                  case 1:
                    WAsmCode[CodeLen >> 1] = AdrLong & 0xffff;
                    CodeLen += 2;
                    break;
                  case 2:
                    WAsmCode[ CodeLen >> 1     ] = AdrLong >> 16;
                    WAsmCode[(CodeLen >> 1) + 1] = AdrLong & 0xffff;
                    CodeLen += 4;
                    break;
                }
              }
            }
          }
        }
      }
    }
  }
}

static void DecodeFixedLong(Word Index)
{
  BitOrder *pOrder = FixedLongOrders + Index;
  if (ArgCnt != 0) WrError(1110);
  else if (*AttrPart != '\0') WrError(1100);
  else if (strcmp(Format, " ")) WrError(1090);
  else
  {
    CodeLen = 10;
    WAsmCode[0] = 0xd20c;
    WAsmCode[1] = pOrder->Code1;
    WAsmCode[2] = pOrder->Code2;
    WAsmCode[3] = 0x9e09;
    WAsmCode[4] = 0x0700;
  }
}

static void DecodeMOV(Word Code)
{
  LongInt HVal;

  UNUSED(Code);

  if (ArgCnt != 2) WrError(1110);
  else if (CheckFormat("GELSZQI"))
  {
    if ((GetOpSize(ArgStr[1], 1)) && (GetOpSize(ArgStr[2], 2)))
    {
      if (OpSize[2] == -1)
        OpSize[2] = 2;
      if (OpSize[1] == -1)
        OpSize[1] = OpSize[2];
      if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
       && (DecodeAdr(ArgStr[2], 2, Mask_AllGen-MModPop)))
      {
        if (FormatCode == 0)
        {
          if (AdrType[1] == ModImm)
          {
            HVal = ImmVal(1);
            if (HVal == 0)
              FormatCode = 5;
            else if ((HVal >= 1) && (HVal <= 8) && (IsShort(2)))
              FormatCode = 6;
            else if ((HVal >= -128) && (HVal <= 127))
              FormatCode = 2;
            else if (IsShort(2))
              FormatCode = 7;
            else
              FormatCode = 1;
          }
          else if ((AdrType[1] == ModReg) && (OpSize[1] == 2) && (IsShort(2)))
            FormatCode = 4;
          else if ((AdrType[2] == ModReg) && (OpSize[2] == 2) && (IsShort(1)))
            FormatCode = 3;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            Make_G(0x8800);
            break;
          case 2:
            Make_E(0x8800, True);
            break;
          case 3:
            if ((!IsShort(1)) || (AdrType[2] != ModReg)) WrError(1350);
            else if (OpSize[2] != 2) WrError(1130);
            else
            {
              WAsmCode[0] = 0x0040 + ((AdrMode[2] & 15) << 10) + (OpSize[1] << 8) + AdrMode[1];
              memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
              CodeLen = 2 + AdrCnt1[1];
            }
            break;
          case 4:
            if ((!IsShort(2)) || (AdrType[1] != ModReg)) WrError(1350);
            else if (OpSize[1] != 2) WrError(1130);
            else
            {
              WAsmCode[0] = 0x0080 + ((AdrMode[1] & 15) << 10) + (OpSize[2] << 8) + AdrMode[2];
              memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
              CodeLen = 2 + AdrCnt1[2];
            }
            break;
          case 5:
            if (AdrType[1] != ModImm) WrError(1350);
            else
            {
              HVal = ImmVal(1);
              if (ChkRange(HVal, 0, 0))
              {
                WAsmCode[0] = 0xc400 + (OpSize[2] << 8) + AdrMode[2];
                memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
                CodeLen = 2 + AdrCnt1[2];
              }
            }
            break;
          case 6:
            if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
            else
            {
              HVal = ImmVal(1);
              if (ChkRange(HVal, 1, 8))
              {
                WAsmCode[0] = 0x6000 + ((HVal & 7) << 10) + (OpSize[2] << 8) + AdrMode[2];
                memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
                CodeLen = 2 + AdrCnt1[2];
              }
            }
            break;
          case 7:
            Make_I(0x48c0, True);
            break;
        }
      }
    }
  }
}

static void DecodeOne(Word Index)
{
  OneOrder *pOrder = OneOrders + Index;

  if (ArgCnt != 1) WrError(1110);
  else if (CheckFormat("G"))
  {
    if (GetOpSize(ArgStr[1], 1))
    {
      if ((OpSize[1] == -1) && (pOrder->OpMask != 0))
        OpSize[1] = DefSize(pOrder->OpMask);
      if ((OpSize[1] != -1) && (((1 << OpSize[1]) & pOrder->OpMask) == 0)) WrError(1130);
      else
      {
        if (DecodeAdr(ArgStr[1], 1, pOrder->Mask))
        {
          /* da nur G, Format ignorieren */
          WAsmCode[0] = pOrder->Code + AdrMode[1];
          if (pOrder->OpMask != 0)
            WAsmCode[0] += OpSize[1] << 8;
          memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
          CodeLen = 2 + AdrCnt1[1];
        }
      }
    }
  }
}

static void DecodeADD_SUB(Word IsSUB)
{
  if (ArgCnt != 2) WrError(1110);
  else if (CheckFormat("GELQI"))
  {
    if ((GetOpSize(ArgStr[2], 2)) && (GetOpSize(ArgStr[1], 1)))
    {
      if (OpSize[2] == -1) OpSize[2] = 2;
      if (OpSize[1] == -1) OpSize[1] = OpSize[2];
      if ((DecodeAdr(ArgStr[1], 1, Mask_Source)) && (DecodeAdr(ArgStr[2], 2, Mask_PureDest)))
      {
        if (FormatCode==0)
        {
          if (AdrType[1] == ModImm)
          {
            LongInt HVal = ImmVal(1);

            if (IsShort(2))
            {
              if ((HVal >= 1) && (HVal <= 8))
                FormatCode = 4;
              else
                FormatCode = 5;
            }
            else if ((HVal >= -128) && (HVal < 127))
              FormatCode = 2;
            else
              FormatCode = 1;
          }
          else if (IsShort(1) && (AdrType[2] == ModReg) && (OpSize[1] == 2) && (OpSize[2] == 2))
            FormatCode = 3;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            Make_G(IsSUB << 11);
            break;
          case 2:
            Make_E(IsSUB << 11, True);
            break;
          case 3:
            if ((!IsShort(1)) || (AdrType[2] != ModReg)) WrError(1350);
            else if ((OpSize[1] != 2) || (OpSize[2] != 2)) WrError(1130);
            else
            {
              WAsmCode[0] = 0x8100 + (IsSUB << 6) + ((AdrMode[2] & 15) << 10) + AdrMode[1];
              memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
              CodeLen = 2 + AdrCnt1[1];
              if ((AdrMode[1] == 0x04) & (AdrMode[2] == 15))
                WrError(140);
            }
            break;
          case 4:
            if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
            else
            {
              LongInt HVal = ImmVal(1);
              if (ChkRange(HVal, 1, 8))
              {
                WAsmCode[0] = 0x4040 + (IsSUB << 13) + ((HVal & 7) << 10) + (OpSize[2] << 8) + AdrMode[2];
                memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
                CodeLen = 2 + AdrCnt1[2];
              }
            }
            break;
          case 5:
            Make_I(0x44c0 + (IsSUB << 11), True);
            break;
        }
      }
    }
  }
}

static void DecodeCMP(Word Code)
{
  LongInt HVal;

  UNUSED(Code);

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("GELZQI"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[2] == -1) OpSize[2] = 2;
    if (OpSize[1] == -1) OpSize[1] = OpSize[2];
    if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
     && (DecodeAdr(ArgStr[2], 2, Mask_NoImmGen-MModPush)))
    {
      if (FormatCode == 0)
      {
        if (AdrType[1] == ModImm)
        {
          HVal = ImmVal(1);
          if (HVal == 0)
            FormatCode = 4;
          else if ((HVal >= 1) && (HVal <= 8) && (IsShort(2)))
            FormatCode = 5;
          else if ((HVal >= -128) && (HVal <= 127))
            FormatCode = 2;
          else if (AdrType[2] == ModReg)
            FormatCode = 3;
          else if (IsShort(2))
            FormatCode = 5;
          else
            FormatCode = 1;
        }
        else if ((IsShort(1)) && (AdrType[2] == ModReg))
          FormatCode = 3;
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          Make_G(0x8000);
          break;
        case 2:
          Make_E(0x8000, True);
          break;
        case 3:
          if ((!IsShort(1)) || (AdrType[2] != ModReg)) WrError(1350);
          else if (OpSize[2] != 2) WrError(1130);
          else
          {
            WAsmCode[0] = ((AdrMode[2] & 15) << 10) + (OpSize[1] << 8) + AdrMode[1];
            memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
            CodeLen = 2 + AdrCnt1[1];
          }
          break;
        case 4:
          if (AdrType[1] != ModImm) WrError(1350);
          else
          {
            HVal = ImmVal(1);
            if (ChkRange(HVal, 0, 0))
            {
              WAsmCode[0] = 0xc000 + (OpSize[2] << 8) + AdrMode[2];
              memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
              CodeLen = 2 + AdrCnt1[2];
            }
          }
          break;
        case 5:
          if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
          else
          {
            HVal = ImmVal(1);
            if (ChkRange(HVal, 1, 8))
            {
              WAsmCode[0] = 0x4000 + (OpSize[2] << 8) + AdrMode[2] + ((HVal & 7) << 10);
              memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
              CodeLen = 2 + AdrCnt1[2];
            }
          }
          break;
        case 6:
          Make_I(0x40c0, True);
          break;
      }
    }
  }
}

static void DecodeGE2(Word Index)
{
  GE2Order *pOrder = GE2Orders + Index;

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("GE"))
        && (GetOpSize(ArgStr[2], 2))
        && (GetOpSize(ArgStr[1], 1)))
  {
    if (OpSize[2] == -1)
      OpSize[2] = DefSize(pOrder->SMask2);
    if (OpSize[1] == -1)
      OpSize[1] = DefSize(pOrder->SMask1);
    if (((pOrder->SMask1 & (1 << OpSize[1])) == 0)
     || ((pOrder->SMask2 & (1 << OpSize[2])) == 0)) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, pOrder->Mask1))
          && (DecodeAdr(ArgStr[2], 2, pOrder->Mask2)))
    {
      if (FormatCode==0)
      {
        if (AdrType[1] == ModImm)
        {
          LongInt HVal = ImmVal(1);

          if ((pOrder->Signed) && (HVal >= -128) && (HVal <= 127))
            FormatCode = 2;
          else if ((!pOrder->Signed) && (HVal >= 0) && (HVal <= 255))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          Make_G(pOrder->Code);
          break;
        case 2:
          Make_E(pOrder->Code, pOrder->Signed);
          break;
      }
    }
  }
}

static void DecodeLog(Word Index)
{
  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("GERI"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[2] == -1)
      OpSize[2] = 2;
    if (OpSize[1] == -1)
      OpSize[1] = OpSize[2];
    if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
     && (DecodeAdr(ArgStr[2], 2, Mask_Dest - MModPush)))
    {
      if (FormatCode == 0)
      {
        if (AdrType[1] == ModImm)
        {
          LongInt HVal = ImmVal(1);

          if ((HVal >= 0) && (HVal <= 255))
            FormatCode = 2;
          else if (IsShort(2))
            FormatCode = 4;
          else
            FormatCode = 1;
        }
        else if ((AdrType[1] == ModReg) && (AdrType[2] == ModReg) && (OpSize[1] == 2) && (OpSize[2] == 2))
          FormatCode = 3;
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          Make_G(0x2000 + (Index << 10));
          break;
        case 2:
          Make_E(0x2000 + (Index << 10), False);
          break;
        case 3:
          if ((AdrType[1] != ModReg) || (AdrType[2] != ModReg)) WrError(1350);
          else if ((OpSize[1] != 2) || (OpSize[2] != 2)) WrError(1130);
          else
          {
            WAsmCode[0] = 0x00c0 + (Index << 8) + (AdrMode[1] & 15) + ((AdrMode[2] & 15) << 10);
            CodeLen = 2;
          }
          break;
        case 4:
          if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
          else
          {
            WAsmCode[0] = 0x50c0 + (OpSize[2] << 8) + (Index << 10) + AdrMode[2];
            memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
            AdaptImm(1, OpSize[2], False);
            memcpy(WAsmCode + 1 + AdrCnt2[2], AdrVals[1], AdrCnt1[1]);
            CodeLen = 2 + AdrCnt1[1] + AdrCnt1[2];
          }
          break;
      }
      if (OpSize[1] > OpSize[2])
        WrError(140);
    }
  }
}

static void DecodeMul(Word Index)
{
  Boolean Unsigned = Index & 1;

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat(Unsigned ? "GE" : "GER"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
    {
      if (OpSize[2] == -1)
        OpSize[2] = 2;
      if (OpSize[1] == -1)
        OpSize[1] = OpSize[2];
      if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
       && (DecodeAdr(ArgStr[2], 2, Mask_PureDest)))
      {
        if (FormatCode == 0)
        {
          if (AdrType[1] == ModImm)
          {
            LongInt HVal = ImmVal(1);
            if ((HVal >= -128 + (Unsigned << 7))
             && (HVal <= 127 + (Unsigned << 7)))
              FormatCode = 2;
            else
              FormatCode = 1;
          }
          else if ((!Unsigned)
                && (AdrType[1] == ModReg) && (OpSize[1] == 2)
                && (AdrType[2] == ModReg) && (OpSize[2] == 2))
            FormatCode = 3;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            Make_G(0x4000 + (Index << 10));
            break;
          case 2:
            Make_E(0x4000 + (Index << 10), !Unsigned);
            break;
          case 3:
            if ((AdrType[1] != ModReg) || (AdrType[2] != ModReg)) WrError(1350);
            else if ((OpSize[1] != 2) || (OpSize[2] != 2)) WrError(1130);
            else
            {
              WAsmCode[0] = 0x00d0
                          + ((AdrMode[2] & 15) << 10)
                          + (Index << 7)
                          + (AdrMode[1] & 15);
              CodeLen = 2;
            }
        }
      }
    }
}

static void DecodeGetPut(Word Code)
{
  Word AdrWord, Mask, Mask2;

  if (Code & 0x80)
  {
    Mask = Mask_Source; Mask2 = MModReg; AdrWord = 1;
  }
  else
  {
    Mask = MModReg; Mask2 = Mask_Dest; AdrWord = 2;
  }
  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1],1))
        && (GetOpSize(ArgStr[2],2)))
  {
    if (OpSize[AdrWord] == -1) OpSize[AdrWord] = Code & 3;
    if (OpSize[3 - AdrWord] == -1) OpSize[3 - AdrWord] = 2;
    if ((OpSize[AdrWord] != (Code & 3)) || (OpSize[3 - AdrWord] != 2)) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask))
          && (DecodeAdr(ArgStr[2],2,Mask2)))
    {
      Make_G(Code & 0xff00);
      WAsmCode[0] += 0x0400;
    }
  }
}

static void DecodeMOVA(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("GR"))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[2] == -1)
      OpSize[2] = 2;
    OpSize[1] = 0;
    if (OpSize[2] != 2) WrError(1110);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_PureMem))
          && (DecodeAdr(ArgStr[2], 2, Mask_Dest)))
    {
      if (FormatCode == 0)
      {
        if ((AdrType[1] == ModDisp16) && (AdrType[2] == ModReg))
          FormatCode = 2;
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          Make_G(0xb400);
          WAsmCode[0] += 0x800;
          break;
        case 2:
          if ((AdrType[1] != ModDisp16) || (AdrType[2] != ModReg)) WrError(1350);
          else
          {
            WAsmCode[0] = 0x03c0 + ((AdrMode[2] & 15) << 10) + (AdrMode[1] & 15);
            WAsmCode[1] = AdrVals[1][0];
            CodeLen = 4;
          }
          break;
      }
    }
  }
}

static void DecodeQINS_QDEL(Word IsQINS)
{
  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("G"))
       &&  ((IsQINS) || (GetOpSize(ArgStr[2], 2))))
  {
    if (OpSize[2] == -1)
      OpSize[2] = 2;
    OpSize[1] = 0;
    if (OpSize[2] != 2) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_PureMem))
          && (DecodeAdr(ArgStr[2], 2, Mask_PureMem | (IsQINS ? 0 : MModReg))))
    {
      Make_G(0xb000 + IsQINS);
      WAsmCode[0] += 0x800;
    }
  }
}

static void DecodeRVBY(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[2] == -1)
      OpSize[2] = 2;
    if (OpSize[1] == -1)
      OpSize[1] = OpSize[2];
    if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
     && (DecodeAdr(ArgStr[2], 2, Mask_Dest)))
    {
      Make_G(0x4000);
      WAsmCode[0] += 0x400;
    }
  }
}

static void DecodeSHL_SHA(Word IsSHA)
{
  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("GEQ"))
       && (GetOpSize(ArgStr[1], 1))
       && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[1] == -1)
      OpSize[1] = 0;
    if (OpSize[2] == -1)
      OpSize[2] = 2;
    if (OpSize[1] != 0)
      WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
          && (DecodeAdr(ArgStr[2], 2, Mask_PureDest)))
    {
      if (FormatCode == 0)
      {
        if (AdrType[1] == ModImm)
        {
          LongInt HVal = ImmVal(1);
          if ((IsShort(2)) && (abs(HVal) >= 1) && (abs(HVal) <= 8) && ((IsSHA == 0) || (HVal < 0)))
            FormatCode = 3;
          else if ((HVal >= -128) && (HVal <= 127))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          Make_G(0x3000 + (IsSHA << 10));
          break;
        case 2:
          Make_E(0x3000 + (IsSHA << 10), True);
          break;
        case 3:
          if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
          else
          {
            LongInt HVal = ImmVal(1);
            if (ChkRange(HVal, -8, (1 - IsSHA) << 3))
            {
              if (HVal == 0) WrError(1135);
              else
              {
                if (HVal < 0)
                  HVal += 16;
                else HVal &= 7;
                WAsmCode[0] = 0x4080 + (HVal << 10) + (IsSHA << 6) + (OpSize[2] << 8) + AdrMode[2];
                memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
                CodeLen = 2 + AdrCnt1[2];
              }
            }
          }
          break;
      }
    }
  }
}

static void DecodeSHXL_SHXR(Word Code)
{
  if (ArgCnt != 1) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 1)))
  {
    if (OpSize[1] == -1)
      OpSize[1] = 2;
    if (OpSize[1] != 2) WrError(1130);
    else if (DecodeAdr(ArgStr[1], 1, Mask_PureDest))
    {
      WAsmCode[0] = 0x02f7;
      WAsmCode[1] = Code + AdrMode[1];
      memcpy(WAsmCode + 1, AdrVals, AdrCnt1[1]);
      CodeLen = 4 + AdrCnt1[1];
    }
  }
}

static void DecodeCHK(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 3) WrError(1110);
  else if (OptionCnt > 1) WrError(1115);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 2))
        && (GetOpSize(ArgStr[2], 1))
        && (GetOpSize(ArgStr[3], 3)))
  {
    Boolean OptOK;
    Word IsSigned;

    if ((OptionCnt == 0)
     || (!strcasecmp(Options[0], "N")))
    {
      OptOK = True;
      IsSigned = 0;
    }
    else if (!strcasecmp(Options[0], "S"))
    {
      OptOK = True;
      IsSigned = 1;
    }
    else
    {
      OptOK = False;
      IsSigned = 0;
    }

    if (OpSize[3] == -1) OpSize[3] = 2;
    if (OpSize[2] == -1) OpSize[2] = OpSize[3];
    if (OpSize[1] == -1) OpSize[1] = OpSize[3];
    if ((OpSize[1] != OpSize[2]) || (OpSize[2] != OpSize[3])) WrError(1131);
    else if (!OptOK) WrXError(1360, Options[0]);
    else if ((DecodeAdr(ArgStr[1], 2, Mask_MemGen-MModPop-MModPush))
          && (DecodeAdr(ArgStr[2], 1, Mask_Source))
          && (DecodeAdr(ArgStr[3], 3, MModReg)))
    {
      OpSize[2] = 2 + IsSigned;
      Make_G((AdrMode[3] & 15) << 10);
      WAsmCode[0] += 0x400;
    }
  }
}

static void DecodeCSI(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 3) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 3))
        && (GetOpSize(ArgStr[2], 1))
        && (GetOpSize(ArgStr[3], 2)))
  {
    if (OpSize[3] == -1) OpSize[3] = 2;
    if (OpSize[2] == -1) OpSize[2] = OpSize[3];
    if (OpSize[1] == -1) OpSize[1] = OpSize[2];
    if ((OpSize[1] != OpSize[2]) || (OpSize[2] != OpSize[3])) WrError(1131);
    else if ((DecodeAdr(ArgStr[1], 3, MModReg))
          && (DecodeAdr(ArgStr[2], 1, Mask_Source))
          && (DecodeAdr(ArgStr[3], 2, Mask_PureMem)))
    {
      OpSize[2] = 0;
      Make_G((AdrMode[3] & 15) << 10);
      WAsmCode[0] += 0x400;
    }
  }
}

static void DecodeDIVX_MULX(Word Code)
{
  if (ArgCnt != 3) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2))
        && (GetOpSize(ArgStr[3], 3)))
  {
    if (OpSize[3] == -1) OpSize[3] = 2;
    if (OpSize[2] == -1) OpSize[2] = OpSize[3];
    if (OpSize[1] == -1) OpSize[1] = OpSize[2];
    if ((OpSize[1] != 2) || (OpSize[2] != 2) || (OpSize[3] != 2)) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
          && (DecodeAdr(ArgStr[2], 2, Mask_PureDest))
          && (DecodeAdr(ArgStr[3], 3, MModReg)))
    {
      OpSize[2] = 0;
      Make_G(Code + ((AdrMode[3] & 15) << 10));
      WAsmCode[0] += 0x400;
    }
  }
}

static void DecodeWAIT(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 1) WrError(1110);
  else if (*AttrPart != '\0') WrError(1100);
  else if (strcmp(Format, " ")) WrError(1090);
  else if (*ArgStr[1] != '#') WrError(1350);
  else
  {
    Boolean OK;

    WAsmCode[1] = EvalIntExpression(ArgStr[1] + 1, UInt3, &OK);
    if (OK)
    {
      WAsmCode[0] = 0x0fd6;
      CodeLen = 4;
    }
  }
}

static void DecodeBit(Word Index)
{
  const BitOrder *pOrder = BitOrders + Index;

  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat(pOrder->Code2 ? "GEQ" : "GE"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[1] == -1)
      OpSize[1] = 2;
    if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
     && (DecodeAdr(ArgStr[2], 2, Mask_PureDest)))
    {
      if (OpSize[2] == -1)
        OpSize[2] = ((AdrType[2] == ModReg) && (!pOrder->MustByte)) ? 2 : 0;

      if (((AdrType[2] != ModReg) || (pOrder->MustByte)) && (OpSize[2] != 0)) WrError(1130);
      else
      {
        if (FormatCode == 0)
        {
          if (AdrType[1] == ModImm)
          {
            LongInt HVal = ImmVal(1);

            if ((HVal >= 0) && (HVal <= 7) && (IsShort(2)) && (pOrder->Code2 != 0) && (OpSize[2] == 0))
              FormatCode = 3;
            else if ((HVal >= -128) && (HVal < 127))
              FormatCode = 2;
            else
              FormatCode = 1;
          }
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            Make_G(pOrder->Code1);
            break;
          case 2:
            Make_E(pOrder->Code1, True);
            break;
          case 3:
            if ((AdrType[1] != ModImm) || (!IsShort(2))) WrError(1350);
            else if (OpSize[2] != 0) WrError(1130);
            else
            {
              LongInt HVal = ImmVal(1);
              if (ChkRange(HVal, 0, 7))
              {
                WAsmCode[0] = pOrder->Code2 + ((HVal & 7) << 10) + AdrMode[2];
                memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
                CodeLen = 2 + AdrCnt1[2];
              }
            }
            break;
        }
      }
    }
  }
}

static void DecodeBField(Word Code)
{
  if (ArgCnt != 4) WrError(1110);
  else if ((CheckBFieldFormat())
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2))
        && (GetOpSize(ArgStr[3], 3))
        && (GetOpSize(ArgStr[4], 4)))
  {
    if (OpSize[1] == -1) OpSize[1] = 2;
    if (OpSize[2] == -1) OpSize[2] = 2;
    if (OpSize[3] == -1) OpSize[3] = 2;
    if (OpSize[4] == -1) OpSize[4] = 2;
    if ((DecodeAdr(ArgStr[1], 1, MModReg | MModImm))
     && (DecodeAdr(ArgStr[3], 3, MModReg | MModImm))
     && (DecodeAdr(ArgStr[2], 2, Mask_Source))
     && (DecodeAdr(ArgStr[4], 4, Mask_PureMem)))
    {
      if (FormatCode == 0)
      {
        if (AdrType[3] == ModReg)
          FormatCode = (AdrType[1] == ModReg) ? 1 : 2;
        else
          FormatCode = (AdrType[1] == ModReg) ? 3 : 4;
      }
      switch (FormatCode)
      {
        case 1:
          if ((AdrType[1] != ModReg) || (AdrType[3] != ModReg)) WrError(1350);
          else if ((OpSize[1] != 2) || (OpSize[3] != 2) || (OpSize[4] != 2)) WrError(1130);
          else
          {
            WAsmCode[0] = 0xd000 + (OpSize[2] << 8) + AdrMode[2];
            memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
            WAsmCode[1 + AdrCnt2[2]] = 0xc200 + (Code << 10) + AdrMode[4];
            memcpy(WAsmCode + 2 + AdrCnt2[2], AdrVals[4], AdrCnt1[4]);
            WAsmCode[2 + AdrCnt2[2] + AdrCnt2[4]] = ((AdrMode[3] & 15) << 10) + (AdrMode[1] & 15);
            CodeLen = 6 + AdrCnt1[2] + AdrCnt1[4];
          }
          break;
        case 2:
          if ((AdrType[1] != ModImm) || (AdrType[3] != ModReg)) WrError(1350);
          else if ((OpSize[3] != 2) || (OpSize[4] != 2)) WrError(1130);
          else
          {
            WAsmCode[0] = 0xd000 + (OpSize[2] << 8) + AdrMode[2];
            memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
            WAsmCode[1 + AdrCnt2[2]] = 0xd200 + (Code << 10) + AdrMode[4];
            memcpy(WAsmCode + 2 + AdrCnt2[2], AdrVals[4], AdrCnt1[4]);
            WAsmCode[2 + AdrCnt2[2] + AdrCnt2[4]] = ((AdrMode[3] & 15) << 10) + (OpSize[1] << 8);
            CodeLen = 6 + AdrCnt1[2] + AdrCnt1[4];
            if (OpSize[1] == 0)
              WAsmCode[(CodeLen-2) >> 1] += AdrVals[1][0] & 0xff;
            else
            {
              memcpy(WAsmCode + (CodeLen >> 1), AdrVals[1], AdrCnt1[1]);
              CodeLen += AdrCnt1[1];
            }
          }
          break;
        case 3:
          if ((AdrType[1] != ModReg) || (AdrType[2] != ModImm) || (AdrType[3] != ModImm)) WrError(1350);
          else if ((OpSize[1] != 2) || (OpSize[4] != 2)) WrError(1130);
          else
          {
            LongInt Offset = ImmVal(2);
            if (ChkRange(Offset, -128, 127))
            {
              LongInt Width = ImmVal(3);
              if (ChkRange(Width, 1, 32))
              {
                WAsmCode[0] = 0xbf00 + (Offset & 0xff);
                WAsmCode[1] = 0xc200 + (Code << 10) + AdrMode[4];
                memcpy(WAsmCode + 2, AdrVals[4], AdrCnt1[4]);
                WAsmCode[2 + AdrCnt2[4]] = ((Width & 31) << 10) + (AdrMode[1] & 15);
                CodeLen =6 + AdrCnt1[4];
              }
            }
          }
          break;
        case 4:
          if ((AdrType[1] != ModImm) || (AdrType[2] != ModImm) || (AdrType[3] != ModImm)) WrError(1350);
          else if (OpSize[4] != 2) WrError(1130);
          else
          {
            LongInt Offset = ImmVal(2);
            if (ChkRange(Offset, -128, 127))
            {
              LongInt Width = ImmVal(3);
              if (ChkRange(Offset, 1, 32))
              {
                WAsmCode[0] = 0xbf00 + (Offset & 0xff);
                WAsmCode[1] = 0xd200 + (Code << 10) + AdrMode[4];
                memcpy(WAsmCode + 2, AdrVals[4], AdrCnt1[4]);
                WAsmCode[2 + AdrCnt2[4]] = ((Width & 31) << 10) + (OpSize[1] << 8);
                CodeLen = 6 + AdrCnt1[4];
                if (OpSize[1] == 0)
                  WAsmCode[(CodeLen - 1) >> 1] += AdrVals[1][0] & 0xff;
                else
                {
                  memcpy(WAsmCode + (CodeLen >> 1), AdrVals[1], AdrCnt1[1]);
                  CodeLen += AdrCnt1[1];
                }
              }
            }
          }
          break;
      }
    }
  }
}

static void DecodeBFEXT_BFEXTU(Word IsEXTU)
{
  if (ArgCnt != 4) WrError(1110);
  else if ((CheckFormat("GE"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2))
        && (GetOpSize(ArgStr[3], 3))
        && (GetOpSize(ArgStr[4], 4)))
  {
    if (OpSize[1] == -1) OpSize[1] = 2;
    if (OpSize[2] == -1) OpSize[2] = 2;
    if (OpSize[3] == -1) OpSize[3] = 2;
    if (OpSize[4] == -1) OpSize[4] = 2;
    if ((DecodeAdr(ArgStr[4], 4, MModReg))
     && (DecodeAdr(ArgStr[3], 3, Mask_MemGen & ~(MModPop | MModPush)))
     && (DecodeAdr(ArgStr[2], 2, MModReg | MModImm)))
    {
      if (DecodeAdr(ArgStr[1],1, (AdrType[2] == ModReg) ? Mask_Source : MModImm))
      {
        if (FormatCode == 0)
          FormatCode =  (AdrType[2]== ModReg) ? 1 : 2;
        switch (FormatCode)
        {
          case 1:
            if ((OpSize[2] != 2) || (OpSize[3] != 2) || (OpSize[4] != 2)) WrError(1130);
            else
            {
              WAsmCode[0] = 0xd000 + (OpSize[1] << 8) + AdrMode[1];
              memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
              WAsmCode[1 + AdrCnt2[1]] = 0xea00 + (IsEXTU << 10) + AdrMode[3];
              memcpy(WAsmCode + 2 + AdrCnt2[1], AdrVals[3], AdrCnt1[3]);
              WAsmCode[2 + AdrCnt2[1] + AdrCnt2[3]] = ((AdrMode[2] & 15) << 10) + (AdrMode[4] & 15);
              CodeLen = 6 + AdrCnt1[1] + AdrCnt1[3];
            }
            break;
          case 2:
            if ((AdrType[1] != ModImm) || (AdrType[2] != ModImm)) WrError(1350);
            else if ((OpSize[3] != 2) || (OpSize[4] != 2)) WrError(1350);
            else
            {
              LongInt Offset = ImmVal(1);
              if (ChkRange(Offset, -128, 127))
              {
                LongInt Width = ImmVal(2);
                if (ChkRange(Width, 1, 32))
                {
                  WAsmCode[0] = 0xbf00 + (Offset & 0xff);
                  WAsmCode[1] = 0xea00 + (IsEXTU << 10) + AdrMode[3];
                  memcpy(WAsmCode + 2, AdrVals[3], AdrCnt1[3]);
                  WAsmCode[2 + AdrCnt2[3]] = ((Width & 31) << 10) + (AdrMode[4] & 15);
                  CodeLen = 6 + AdrCnt1[3];
                }
              }
            }
            break;
        }
      }
    }
  }
}

static void DecodeBSCH(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 2) WrError(1110);
  else if (OptionCnt != 1) WrError(1115);
  else if ((strcmp(Options[0], "0")) && (strcmp(Options[0], "1"))) WrXError(1360, Options[0]);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[1] == -1) OpSize[1] = 2;
    if (OpSize[2] == -1) OpSize[2] = 2;
    if (OpSize[1] != 2) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_Source))
          && (DecodeAdr(ArgStr[2], 2, Mask_PureDest)))
    {
      /* immer G-Format */
      WAsmCode[0] = 0xd600 + AdrMode[1];
      memcpy(WAsmCode + 1, AdrVals[1], AdrCnt1[1]);
      WAsmCode[1 + AdrCnt2[1]] = Code + ((Options[0][0] - '0') << 10)+ (OpSize[2] << 8) + AdrMode[2];
      memcpy(WAsmCode + 2 + AdrCnt2[1], AdrVals[2], AdrCnt1[2]);
      CodeLen = 4 + AdrCnt1[1] + AdrCnt1[2];
    }
  }
}

static void DecodeBSR_BRA(Word IsBSR)
{
  if (ArgCnt != 1) WrError(1110);
  else if ((CheckFormat("GD"))
        && (GetOpSize(ArgStr[1], 1)))
  {
    Boolean OK;
    LongInt AdrLong = EvalIntExpression(ArgStr[1], Int32, &OK);
    if (OK)
    {
      DecideBranch(AdrLong, 1);
      switch (FormatCode)
      {
        case 2:
          if (OpSize[1] != -1) WrError(1100);
          else
          {
            AdrLong -= EProgCounter();
            if ((!SymbolQuestionable) && ((AdrLong < -256) || (AdrLong>254))) WrError(1370);
            else if (Odd(AdrLong)) WrError(1375);
            else
            {
              CodeLen = 2;
              WAsmCode[0] = 0xae00 + (IsBSR << 8) + Lo(AdrLong >> 1);
            }
          }
          break;
        case 1:
          WAsmCode[0] = 0x20f7 + (IsBSR << 11) + (((Word)OpSize[1]) << 8);
          AdrLong -= EProgCounter();
          switch (OpSize[1])
          {
            case 0:
              if ((!SymbolQuestionable) && ((AdrLong < -128) || (AdrLong > 127))) WrError(1370);
              else
              {
                CodeLen = 4;
                WAsmCode[1] = Lo(AdrLong);
              }
              break;
            case 1:
              if ((!SymbolQuestionable) && ((AdrLong < -32768) || (AdrLong > 32767))) WrError(1370);
              else
              {
                CodeLen = 4;
                WAsmCode[1] = AdrLong & 0xffff;
              }
              break;
            case 2:
              CodeLen = 6;
              WAsmCode[1] = AdrLong >> 16;
              WAsmCode[2] = AdrLong & 0xffff;
              break;
          }
          break;
      }
    }
  }
}

static void DecodeBcc(Word Code)
{
  if (ArgCnt != 1) WrError(1110);
  else if ((CheckFormat("GD"))
        && (GetOpSize(ArgStr[1], 1)))
  {
    Boolean OK;
    LongInt AdrLong = EvalIntExpression(ArgStr[1], Int32, &OK);
    if (OK)
    {
      DecideBranch(AdrLong, 1);
      switch (FormatCode)
      {
        case 2:
          if (OpSize[1] != -1) WrError(1100);
          else
          {
            AdrLong -= EProgCounter();
            if ((!SymbolQuestionable) && ((AdrLong < -256) || (AdrLong > 254))) WrError(1370);
            else if (Odd(AdrLong)) WrError(1375);
            else
            {
              CodeLen = 2;
              WAsmCode[0] = 0x8000 + Code + Lo(AdrLong >> 1);
            }
          }
          break;
        case 1:
          WAsmCode[0] = 0x00f6 + Code + (((Word)OpSize[1]) << 8);
          AdrLong -= EProgCounter();
          switch (OpSize[1])
          {
            case 0:
              if ((AdrLong < -128) || (AdrLong > 127)) WrError(1370);
              else
              {
                CodeLen = 4;
                WAsmCode[1] = Lo(AdrLong);
              }
              break;
            case 1:
              if ((AdrLong < -32768) || (AdrLong > 32767)) WrError(1370);
              else
              {
                CodeLen = 4;
                WAsmCode[1] = AdrLong & 0xffff;
              }
              break;
            case 2:
              CodeLen = 6;
              WAsmCode[1] = AdrLong >> 16;
              WAsmCode[2] = AdrLong & 0xffff;
              break;
          }
          break;
      }
    }
  }
}

static void DecodeTRAP(Word Code)
{
  Word Condition;

  UNUSED(Code);

  if (ArgCnt != 0) WrError(1110);
  else if (OptionCnt != 1) WrError(1115);
  else if (!DecodeCondition(Options[0], &Condition)) WrXError(1360, Options[0]);
  else
  {
    CodeLen = 2;
    WAsmCode[0] = 0x03d4 | (Condition << 10);
  }
}

static void DecodeTRAPA(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 1) WrError(1110);
  else if (*AttrPart != '\0') WrError(1100);
  else if (strcmp(Format, " ")) WrError(1090);
  else if (*ArgStr[1] != '#') WrError(1350);
  else
  {
    Word AdrWord;
    Boolean OK;

    AdrWord = EvalIntExpression(ArgStr[1] + 1, UInt4, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[0] = 0x03d5 + (AdrWord << 10);
    }
  }
}

static void DecodeENTER_EXITD(Word IsEXITD)
{
  if (ArgCnt!=2) WrError(1110);
  else
  {
    char *pRegList = IsEXITD ? ArgStr[1] : ArgStr[2],
         *pSizeArg = IsEXITD ? ArgStr[2] : ArgStr[1];

    if ((CheckFormat("GE"))
     && (GetOpSize(pSizeArg, 1))
     && (GetOpSize(pRegList, 2)))
    {
      Word RegList;

      if (OpSize[1] == -1) OpSize[1] = 2;
      if (OpSize[2] == -1) OpSize[2] = 2;
      if (OpSize[2] != 2) WrError(1130);
      else if ((DecodeAdr(pSizeArg, 1, MModReg | MModImm))
            && (DecodeRegList(pRegList, &RegList, IsEXITD)))
      {
        if ((RegList & 0xc000) != 0) WrXError(1410,"SP/FP");
        else
        {
          if (FormatCode == 0)
          {
            if (AdrType[1] == ModImm)
            {
              LongInt HVal = ImmVal(1);
              if ((HVal >= -128) && (HVal <= 127))
                FormatCode = 2;
              else
                FormatCode = 1;
            }
            else
              FormatCode = 1;
          }
          switch (FormatCode)
          {
            case 1:
              WAsmCode[0] = 0x02f7;
              WAsmCode[1] = 0x8c00 + (IsEXITD << 12) + (OpSize[1] << 8) + AdrMode[1];
              memcpy(WAsmCode + 2, AdrVals[1], AdrCnt1[1]);
              WAsmCode[2 + AdrCnt2[1]] = RegList;
              CodeLen = 6 + AdrCnt1[1];
              break;
            case 2:
              if (AdrType[1] != ModImm) WrError(1350);
              else
              {
                LongInt HVal = ImmVal(1);
                if (ChkRange(HVal, -128, 127))
                {
                  WAsmCode[0] = 0x8e00 + (IsEXITD << 12) + (HVal & 0xff);
                  WAsmCode[1] = RegList;
                  CodeLen = 4;
                }
              }
              break;
          }
        }
      }
    }
  }
}

static void DecodeSCMP(Word Code)
{
  UNUSED(Code);

  if (DOpSize == -1) DOpSize = 2;
  if (ArgCnt != 0) WrError(1110);
  else if (OptionCnt > 1) WrError(1115);
  else
  {
    Boolean OK = True;
    Word Condition = 6;

    if (OptionCnt == 1)
      OK = DecodeStringCondition(Options[0], &Condition);

    if (!OK) WrXError(1360, Options[0]);
    else
    {
      WAsmCode[0] = 0x00e0 + (DOpSize << 8) + (Condition << 10);
      CodeLen = 2;
    }
  }
}

static void DecodeSMOV_SSCH(Word Code)
{
  if (DOpSize == -1) DOpSize = 2;
  if (ArgCnt != 0) WrError(1110);
  else
  {
    int z;
    Word Condition = 6, Mask = 0;
    Boolean OK = True;

    for (z = 0; z < OptionCnt; z++)
    {
      if (!strcasecmp(Options[z], "F"))
        Mask = 0;
      else if (!strcasecmp(Options[z], "B"))
        Mask = 1;
      else
        OK = DecodeStringCondition(Options[z], &Condition);
      if (!OK)
        break;
    }
    if (!OK) WrXError(1360, Options[z]);
    else
    {
      WAsmCode[0] = 0x00e4 + (DOpSize << 8) + (Condition << 10) + Mask + Code;
      CodeLen = 2;
    }
  }
}

static void DecodeSSTR(Word Code)
{
  UNUSED(Code);

  if (DOpSize == -1) DOpSize = 2;
  if (ArgCnt != 0) WrError(1110);
  else
  {
    WAsmCode[0] = 0x24f7 + (DOpSize << 8);
    CodeLen = 2;
  }
}

static void DecodeLDM_STM(Word Code)
{
  if (ArgCnt != 2) WrError(1110);
  else if (CheckFormat("G"))
  {
    Word Mask = MModIReg | MModDisp16 | MModDisp32 | MModAbs16 | MModAbs32 | MModPCRel16 | MModPCRel32;
    char *pRegList, *pMemArg;
    Word RegList;

    if (Code)
    {
      Mask |= MModPop;
      pRegList = ArgStr[2];
      pMemArg = ArgStr[1];
    }
    else
    {
      Mask |= MModPush;
      pRegList = ArgStr[1];
      pMemArg = ArgStr[2];
    }
    if ((GetOpSize(pRegList, 1))
     && (GetOpSize(pMemArg, 2)))
    {
      if (OpSize[1] == -1) OpSize[1] = 2;
      if (OpSize[2] == -1) OpSize[2] = 2;
      if ((OpSize[1] != 2) || (OpSize[2] != 2)) WrError(1130);
      else if ((DecodeAdr(pMemArg, 2, Mask))
            && (DecodeRegList(pRegList, &RegList, AdrType[2] != ModPush)))
       {
         WAsmCode[0] = 0x8a00 + Code + AdrMode[2];
         memcpy(WAsmCode + 1, AdrVals[2], AdrCnt1[2]);
         WAsmCode[1 + AdrCnt2[2]] = RegList;
         CodeLen = 4 + AdrCnt1[2];
       }
    }
  }
}

static void DecodeSTC_STP(Word Code)
{
  if (ArgCnt != 2) WrError(1110);
  else if ((CheckFormat("G"))
        && (GetOpSize(ArgStr[1], 1))
        && (GetOpSize(ArgStr[2], 2)))
  {
    if (OpSize[2] == -1) OpSize[2] = 2;
    if (OpSize[1] == -1) OpSize[1] = OpSize[1];
    if (OpSize[1] != OpSize[2]) WrError(1132);
    else if ((!Code) && (OpSize[2] != 2)) WrError(1130);
    else if ((DecodeAdr(ArgStr[1], 1, Mask_PureMem))
          && (DecodeAdr(ArgStr[2], 2, Mask_Dest)))
    {
      OpSize[1] = 0;
      Make_G(0xa800 + Code);
      WAsmCode[0] += 0x800;
    }
  }
}

static void DecodeJRNG(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 1) WrError(1110);
  else if ((CheckFormat("GE"))
        && (GetOpSize(ArgStr[1], 1)))
  {
    if (OpSize[1] == -1) OpSize[1] = 1;
    if (OpSize[1] != 1) WrError(1130);
    else if (DecodeAdr(ArgStr[1], 1, MModReg | MModImm))
    {
      if (FormatCode == 0)
      {
        if (AdrType[1] == ModImm)
        {
          LongInt HVal = ImmVal(1);
          if ((HVal >= 0) && (HVal <= 255))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        else
          FormatCode = 1;
      }
      switch (FormatCode)
      {
        case 1:
          WAsmCode[0] = 0xba00 + AdrMode[1];
          memcpy(WAsmCode + 1 , AdrVals[1], AdrCnt1[1]);
          CodeLen = 2 + AdrCnt1[1];
          break;
        case 2:
          if (AdrType[1] != ModImm) WrError(1350);
          else
          {
            LongInt HVal = ImmVal(1);
            if (ChkRange(HVal, 0, 255))
            {
              WAsmCode[0] = 0xbe00 + (HVal & 0xff);
              CodeLen = 2;
            }
          }
          break;
      }
    }
  }
}

/*------------------------------------------------------------------------*/

static void AddFixed(char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddFixedLong(char *NName, Word NCode1, Word NCode2)
{
  if (InstrZ >= FixedLongOrderCount) exit(255);
  FixedLongOrders[InstrZ].Code1 = NCode1;
  FixedLongOrders[InstrZ].Code2 = NCode2;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixedLong);
}

static void AddOne(char *NName, Byte NOpMask, Word NMask, Word NCode)
{
  if (InstrZ >= OneOrderCount) exit(255);
  OneOrders[InstrZ].Code = NCode;
  OneOrders[InstrZ].Mask = NMask;
  OneOrders[InstrZ].OpMask = NOpMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeOne);
}

static void AddGE2(char *NName, Word NMask1, Word NMask2,
                   Byte NSMask1, Byte NSMask2, Word NCode,
                   Boolean NSigned)
{
  if (InstrZ >= GE2OrderCount) exit(255);
  GE2Orders[InstrZ].Mask1 = NMask1;
  GE2Orders[InstrZ].Mask2 = NMask2;
  GE2Orders[InstrZ].SMask1 = NSMask1;
  GE2Orders[InstrZ].SMask2 = NSMask2;
  GE2Orders[InstrZ].Code = NCode;
  GE2Orders[InstrZ].Signed = NSigned;
  AddInstTable(InstTable, NName, InstrZ++, DecodeGE2);
}

static void AddBit(char *NName, Boolean NMust, Word NCode1, Word NCode2)
{
  if (InstrZ >= BitOrderCount) exit(255);
  BitOrders[InstrZ].Code1 = NCode1;
  BitOrders[InstrZ].Code2 = NCode2;
  BitOrders[InstrZ].MustByte = NMust;
  AddInstTable(InstTable, NName, InstrZ++, DecodeBit);
}

static void AddGetPut(char *NName, Byte NSize, Word NCode, Boolean NTurn)
{
  AddInstTable(InstTable, NName, NCode | NSize | (NTurn << 7), DecodeGetPut);
}

static void Addcc(char *BName)
{
  Conditions[InstrZ] = BName + 1;
  AddInstTable(InstTable, BName, InstrZ << 10, DecodeBcc);
  InstrZ++;
}

static void InitFields(void)
{
  Format = (char*)malloc(sizeof(char) * STRINGSIZE);

  InstTable = CreateInstTable(301);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "ADD", 0, DecodeADD_SUB);
  AddInstTable(InstTable, "SUB", 1, DecodeADD_SUB);
  AddInstTable(InstTable, "ACB", 0, DecodeACB_SCB);
  AddInstTable(InstTable, "SCB", 1, DecodeACB_SCB);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP);
  AddInstTable(InstTable, "MOVA", 0, DecodeMOVA);
  AddInstTable(InstTable, "QINS", 1 << 11, DecodeQINS_QDEL);
  AddInstTable(InstTable, "QDEL", 0 << 11, DecodeQINS_QDEL);
  AddInstTable(InstTable, "RVBY", 0, DecodeRVBY);
  AddInstTable(InstTable, "SHL", 0, DecodeSHL_SHA);
  AddInstTable(InstTable, "SHA", 1, DecodeSHL_SHA);
  AddInstTable(InstTable, "SHXL", 0x8a00, DecodeSHXL_SHXR);
  AddInstTable(InstTable, "SHXR", 0x9a00, DecodeSHXL_SHXR);
  AddInstTable(InstTable, "CHK", 0, DecodeCHK);
  AddInstTable(InstTable, "CHK/", 0, DecodeCHK);
  AddInstTable(InstTable, "CSI", 0, DecodeCSI);
  AddInstTable(InstTable, "DIVX", 0x8300, DecodeDIVX_MULX);
  AddInstTable(InstTable, "MULX", 0x8200, DecodeDIVX_MULX);
  AddInstTable(InstTable, "WAIT", 0, DecodeWAIT);
  AddInstTable(InstTable, "BFEXT", 0, DecodeBFEXT_BFEXTU);
  AddInstTable(InstTable, "BFEXTU", 1, DecodeBFEXT_BFEXTU);
  AddInstTable(InstTable, "BSCH/", 0x5000, DecodeBSCH);
  AddInstTable(InstTable, "BSR", 1, DecodeBSR_BRA);
  AddInstTable(InstTable, "BRA", 0, DecodeBSR_BRA);
  AddInstTable(InstTable, "TRAPA", 0, DecodeTRAPA);
  AddInstTable(InstTable, "TRAP/", 0, DecodeTRAP);
  AddInstTable(InstTable, "ENTER", 0, DecodeENTER_EXITD);
  AddInstTable(InstTable, "EXITD", 1, DecodeENTER_EXITD);
  AddInstTable(InstTable, "SCMP", 0, DecodeSCMP);
  AddInstTable(InstTable, "SCMP/", 0, DecodeSCMP);
  AddInstTable(InstTable, "SMOV", 0, DecodeSMOV_SSCH);
  AddInstTable(InstTable, "SMOV/", 0, DecodeSMOV_SSCH);
  AddInstTable(InstTable, "SSCH", 16, DecodeSMOV_SSCH);
  AddInstTable(InstTable, "SSCH/", 16, DecodeSMOV_SSCH);
  AddInstTable(InstTable, "SSTR", 0, DecodeSSTR);
  AddInstTable(InstTable, "LDM", 0x1000, DecodeLDM_STM);
  AddInstTable(InstTable, "STM", 0x0000, DecodeLDM_STM);
  AddInstTable(InstTable, "STC", 0x0000, DecodeSTC_STP);
  AddInstTable(InstTable, "STP", 0x0400, DecodeSTC_STP);
  AddInstTable(InstTable, "JRNG", 0, DecodeJRNG);

  AddFixed("NOP"  , 0x1bd6); AddFixed("PIB"  , 0x0bd6);
  AddFixed("RIE"  , 0x08f7); AddFixed("RRNG" , 0x3bd6);
  AddFixed("RTS"  , 0x2bd6); AddFixed("STCTX", 0x07d6);
  AddFixed("REIT" , 0x2fd6);

  FixedLongOrders = (BitOrder*) malloc(sizeof(BitOrder) * FixedLongOrderCount); InstrZ = 0;
  AddFixedLong("STOP", 0x5374, 0x6f70);
  AddFixedLong("SLEEP", 0x5761, 0x6974);

  OneOrders = (OneOrder *) malloc(sizeof(OneOrder) * OneOrderCount); InstrZ = 0;
  AddOne("ACS"   , 0x00, Mask_PureMem,                    0x8300);
  AddOne("NEG"   , 0x07, Mask_PureDest,                   0xc800);
  AddOne("NOT"   , 0x07, Mask_PureDest,                   0xcc00);
  AddOne("JMP"   , 0x00, Mask_PureMem,                    0x8200);
  AddOne("JSR"   , 0x00, Mask_PureMem,                    0xaa00);
  AddOne("LDCTX" , 0x00, MModIReg | MModDisp16 | MModDisp32 |
                         MModAbs16 | MModAbs32 | MModPCRel16 | MModPCRel32, 0x8600);
  AddOne("LDPSB" , 0x02, Mask_Source,                     0xdb00);
  AddOne("LDPSM" , 0x02, Mask_Source,                     0xdc00);
  AddOne("POP"   , 0x04, Mask_PureDest,                   0x9000);
  AddOne("PUSH"  , 0x04, Mask_Source-MModPop,             0xb000);
  AddOne("PUSHA" , 0x00, Mask_PureMem,                    0xa200);
  AddOne("STPSB" , 0x02, Mask_Dest,                       0xdd00);
  AddOne("STPSM" , 0x02, Mask_Dest,                       0xde00);

  GE2Orders = (GE2Order *) malloc(sizeof(GE2Order) * GE2OrderCount); InstrZ = 0;
  AddGE2("ADDU" , Mask_Source, Mask_PureDest, 7, 7, 0x0400, False);
  AddGE2("ADDX" , Mask_Source, Mask_PureDest, 7, 7, 0x1000, True );
  AddGE2("SUBU" , Mask_Source, Mask_PureDest, 7, 7, 0x0c00, False);
  AddGE2("SUBX" , Mask_Source, Mask_PureDest, 7, 7, 0x1800, True );
  AddGE2("CMPU" , Mask_Source, Mask_PureDest|MModPop, 7, 7, 0x8400, False);
  AddGE2("LDC"  , Mask_Source, Mask_PureDest, 7, 4, 0x9800, True );
  AddGE2("LDP"  , Mask_Source, Mask_PureMem , 7, 7, 0x9c00, True );
  AddGE2("MOVU" , Mask_Source, Mask_Dest    , 7, 7, 0x8c00, True );
  AddGE2("REM"  , Mask_Source, Mask_PureDest, 7, 7, 0x5800, True );
  AddGE2("REMU" , Mask_Source, Mask_PureDest, 7, 7, 0x5c00, True );
  AddGE2("ROT"  , Mask_Source, Mask_PureDest, 1, 7, 0x3800, True );

  BitOrders = (BitOrder *) malloc(sizeof(BitOrder) * BitOrderCount); InstrZ = 0;
  AddBit("BCLR" , False, 0xb400, 0xa180);
  AddBit("BCLRI", True , 0xa400, 0x0000);
  AddBit("BNOT" , False, 0xb800, 0x0000);
  AddBit("BSET" , False, 0xb000, 0x8180);
  AddBit("BSETI", True , 0xa000, 0x81c0);
  AddBit("BTST" , False, 0xbc00, 0xa1c0);

  AddGetPut("GETB0", 0, 0xc000, False);
  AddGetPut("GETB1", 0, 0xc400, False);
  AddGetPut("GETB2", 0, 0xc800, False);
  AddGetPut("GETH0", 1, 0xcc00, False);
  AddGetPut("PUTB0", 0, 0xd000, True );
  AddGetPut("PUTB1", 0, 0xd400, True );
  AddGetPut("PUTB2", 0, 0xd800, True );
  AddGetPut("PUTH0", 1, 0xdc00, True );

  InstrZ = 0;
  AddInstTable(InstTable, "BFCMP" , InstrZ++, DecodeBField);
  AddInstTable(InstTable, "BFCMPU", InstrZ++, DecodeBField);
  AddInstTable(InstTable, "BFINS" , InstrZ++, DecodeBField);
  AddInstTable(InstTable, "BFINSU", InstrZ++, DecodeBField);

  InstrZ = 0;
  AddInstTable(InstTable, "MUL" , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "MULU", InstrZ++, DecodeMul);
  AddInstTable(InstTable, "DIV" , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "DIVU", InstrZ++, DecodeMul);

  InstrZ = 0; Conditions = (char**)malloc(ConditionCount * sizeof(char*));
  Addcc("BXS");
  Addcc("BXC");
  Addcc("BEQ");
  Addcc("BNE");
  Addcc("BLT");
  Addcc("BGE");
  Addcc("BLE");
  Addcc("BGT");
  Addcc("BVS");
  Addcc("BVC");
  Addcc("BMS");
  Addcc("BMC");
  Addcc("BFS");
  Addcc("BFC");

  InstrZ = 0;
  AddInstTable(InstTable, "AND", InstrZ++, DecodeLog);
  AddInstTable(InstTable, "OR" , InstrZ++, DecodeLog);
  AddInstTable(InstTable, "XOR", InstrZ++, DecodeLog);
}

static void DeinitFields(void)
{
  free(Conditions);
  free(Format);
  free(FixedLongOrders);
  free(OneOrders);
  free(GE2Orders);
  free(BitOrders);
  DestroyInstTable(InstTable);
}

/*------------------------------------------------------------------------*/

static void MakeCode_M16(void)
{
  int z;
  char *p;

  DOpSize = -1;
  for (z = 1; z <= ArgCnt; OpSize[z++] = -1);

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Formatangabe abspalten */

  switch (AttrSplit)
  {
    case '.':
      p = strchr(AttrPart, ':');
      if (p)
      {
        if (p < AttrPart + strlen(AttrPart) - 1)
          strmaxcpy(Format, p + 1, STRINGSIZE - 1);
        else
          strcpy(Format, " ");
        *p = '\0';
      }
      else
        strcpy(Format, " ");
      break;
    case ':':
      p = strchr(AttrPart, '.');
      if (!p)
      {
        strmaxcpy(Format, AttrPart, STRINGSIZE - 1);
        *AttrPart = '\0';
      }
      else
      {
        *p = '\0';
        if (p == AttrPart)
          strcpy(Format, " ");
        else
          strmaxcpy(Format, AttrPart, STRINGSIZE - 1);
      }
      break;
    default:
      strcpy(Format," ");
  }
  NLS_UpString(Format);

  /* Attribut abarbeiten */

  if (*AttrPart == '\0')
    DOpSize = -1;
  else
    switch (mytoupper(*AttrPart))
    {
      case 'B':
        DOpSize = 0; break;
      case 'H':
        DOpSize = 1; break;
      case 'W':
        DOpSize = 2; break;
      default:
        WrError(1107); return;
    }

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  SplitOptions();

  if (!LookupInstTable(InstTable, OpPart))
    WrXError(1200, OpPart);
}

static Boolean IsDef_M16(void)
{
  return False;
}

static void SwitchFrom_M16(void)
{
  DeinitFields();
}

static void SwitchTo_M16(void)
{
  TurnWords = True;
  ConstMode = ConstModeIntel;
  SetIsOccupied = False;

  PCSymbol = "$";
  HeaderID = 0x13;
  NOPCode = 0x1bd6;
  DivideChars=",";
  HasAttrs = True;
  AttrChars = ".:";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = INTCONST_ffffffff;

  MakeCode = MakeCode_M16;
  IsDef = IsDef_M16;
  SwitchFrom = SwitchFrom_M16;
  InitFields();
}

void codem16_init(void)
{
  CPUM16 = AddCPU("M16", SwitchTo_M16);
}
