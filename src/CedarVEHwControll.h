/*
 *      Copyright (C) 2013 Edgar Hucek
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _CEDARVEHWCONTROLL_H_
#define _CEDARVEHWCONTROLL_H_

#if defined(HAVE_LIBCEDAR)

#include "libvecore/libve_typedef.h"
#include "libvecore/libve_adapter.h"
#include "cedardev_api.h"
#include <stdint.h>
#include <pthread.h>

class CCedarVEHwControll
{
public:
  CCedarVEHwControll();
  virtual ~CCedarVEHwControll();
  virtual void VeResetHardware();
  static void VeResetHardwareStatic();
  virtual void VeEnableClock(u8 enable, u32 speed);
  static void VeEnableClockStatic(u8 enable, u32 speed);
  virtual void VeEnableIntr(u8 enable);
  static void VeEnableIntrStatic(u8 enable);
  virtual uint64_t VeGetAVS();
  virtual s32 VeWaitIntr();
  static s32 VeWaitIntrStatic();
  virtual u32 VeGetRegBaseAddr();
  static u32 VeGetRegBaseAddrStatic();
  virtual memtype_e VeGetMemType();
  static memtype_e VeGetMemTypeStatic();
  static void *MemAllocStatic(u32 size);
  static void MemFreeStatic(void* p);
  static void *MemPallocStatic(u32 size, u32 align);
  static void MemPfreeStatic(void* p);
  static void MemSetStatic(void* mem, u32 value, u32 size);
  static void MemCpyStatic(void* dst, void* src, u32 size);
  void MemFlushCache(u8* mem, u32 size);
  static void MemFlushCacheStatic(u8* mem, u32 size);
  static u32 MemGetPhyAddrStatic(u32 virtual_addr);
  static s32 SysPrintStatic(u8* func, u32 line, ...);
  static void SysSleepStatic(u32 ms);
  virtual void FlushCache(u8 *start, u8 *end);
  virtual void Close();
  virtual bool Open();
  virtual void Lock();
  virtual void UnLock();
private:
protected:
  bool          m_open;
  pthread_mutex_t m_lock;
  int           m_decoderHandle;
  cedarv_env_info_t m_cedarEnvInfo;
  int64_t       m_wallClock;
  unsigned int  m_avsCntLast;
};

extern CCedarVEHwControll g_CedarVEHwControll;

#endif

#endif
